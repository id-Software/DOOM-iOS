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


/*
==============================================================================
  Carousel -
  
 This UI Object must be setup by Interface builder. 
 
 items must be added by code.
 
==============================================================================
*/

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "Label.h"

@interface idCarouselItem : NSObject
{
@public
    
    UIImage *           image;          // Image Of the Item.
    NSString *          title;          // Title of the Item.
    int                 hash;           // Optional Item Hash Value.
}

@property (nonatomic, retain) NSString *title;
@property (nonatomic, retain) UIImage *image;
@property (nonatomic, assign) int hash;

@end


@interface idCarousel : UIControl {
@public
    
    NSInteger                         currentItem;        // The Current Carousel Item.
    NSInteger                         prevItem;           // The Prev Carousel Item.
    NSInteger                         nextItem;           // The Next Carousel Item.
    
@private
    IBOutlet UIImageView *      mainSelection;      // Renderable Main Selection
    IBOutlet UIImageView *      prevSelection;      // Renderable Prev Selection
    IBOutlet UIImageView *      nextSelection;      // Renderable Next Selection
    IBOutlet idLabel *          selectionLabel;     // Title Label.
        
    CGRect                      mainRect;           // Main Selection Layout Rect.
    CGRect                      prevRect;           // Prev Selection Layout Rect.
    CGRect                      nextRect;           // Next Selection Layout Rect.
    
    NSMutableArray *            carouselItems;      // All the Available Items to choose from.
    
    int                         hasDragged;         // if the User has Dragged for a revolution.
    CGPoint                     startDragPoint;     // the starting point where the user has touched.
}

- (void)        Init;
- (void)        AddCarouselItem:( NSString* )Image
							   :( NSString* )Title
							   :( int )ItemHash;

- (NSInteger)  GetSelectedItem_Index;
- (int)         GetSelectedItem_Hash;

- (IBAction)    MoveForward;
- (IBAction)    MoveBackward;

- (void)        RotateToOrigin;
- (void)        RotateForward;
- (void)        RotateBackward;

@end
