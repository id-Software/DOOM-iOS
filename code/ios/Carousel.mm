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
#import "Carousel.h"
#include <algorithm>
@implementation idCarouselItem

@synthesize image;
@synthesize title;
@synthesize hash;

@end

@implementation idCarousel

// Move full turn at this percentage
const static float CAROUSEL_CHANGE_DRAG =  0.2f;
const static float CAROUSEL_BACKITEMS_ALPHA = 0.5f;
const static float CAROUSEL_FRONTITEM_ALPHA = 1.0f;

enum idCarouselDrag_t {
    CAROUSEL_DRAG_MOVE_NONE = 0,
    CAROUSEL_DRAG_MOVE_FORWARD,
    CAROUSEL_DRAG_MOVE_BACKWARD
};

/*
 ========================
 Lerp
 ========================
 */
static CGFloat Lerp(CGFloat from, CGFloat to, CGFloat f )
{
	return from + ( ( to - from ) * f );
}

/*
 ========================
 LerpPoint
 ========================
 */
static CGPoint LerpPoint(CGPoint a, CGPoint b, CGFloat t)
{
    CGPoint p;
	p.x = Lerp(a.x, b.x, t);
	p.y = Lerp(a.y, b.y, t);
	return p;
}

/*
 ========================
 LerpSize
 ========================
 */
static CGSize LerpSize(CGSize a, CGSize b, CGFloat t)
{
    CGSize s;
	s.width = Lerp(a.width, b.width, t);
	s.height = Lerp(a.height, b.height, t);
	return s;
}

/*
 ========================
 LerpRect
 ========================
 */
static CGRect  LerpRect( CGRect a, CGRect b, CGFloat t ) {
 
    CGRect r;
    r.origin = LerpPoint( a.origin, b.origin, t );
    r.size = LerpSize( a.size, b.size, t );
    return r;
    
}

/*
 ========================
 awakeFromNib
 ========================
 */
- (void) awakeFromNib {
 
    mainRect = mainSelection.frame;
    prevRect = prevSelection.frame;
    nextRect = nextSelection.frame;
    
    // Allocate our Items Array
    carouselItems = [[ NSMutableArray alloc ] init ];
    
    [super awakeFromNib];
}

/*
 ========================
 touchesBegan
 ========================
 */
- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
    (void)(event);
    hasDragged = 0;
    startDragPoint = [[touches anyObject] locationInView:self];
}

/*
 ========================
 touchesMoved
 ========================
 */
- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    (void)(event);
    if( prevSelection == nil || nextSelection == nil ) {
        // Nothing to transition to... dont swip transition
        return;
    }
    
    CGPoint movedPoint = [[touches anyObject] locationInView:self];
    CGFloat offsetX = movedPoint.x - startDragPoint.x;
    
    // Find out where im at relative to the side of the screen
    CGFloat percentage = offsetX / self.frame.size.width;
    percentage = (float)fmin( percentage, 1.0f );
    percentage = (float)fmax( percentage, -1.0f );
    
    if( percentage > CAROUSEL_CHANGE_DRAG ) {
        hasDragged = CAROUSEL_DRAG_MOVE_FORWARD;
    } else if( percentage < -CAROUSEL_CHANGE_DRAG ) {
        hasDragged = CAROUSEL_DRAG_MOVE_BACKWARD;
    }

    if( percentage > 0 ) { // Going to the right of the screen..
        
        CGRect newMainRect = LerpRect( mainRect, nextRect , percentage );
        CGRect newNextRect = LerpRect( nextRect, prevRect , percentage );
        CGRect newPrevRect = LerpRect( prevRect, mainRect , percentage );
        
        mainSelection.frame = newMainRect;
        prevSelection.frame = newPrevRect;
        nextSelection.frame = newNextRect;
        
        mainSelection.alpha = Lerp( CAROUSEL_FRONTITEM_ALPHA, CAROUSEL_BACKITEMS_ALPHA, percentage );
        prevSelection.alpha = Lerp( CAROUSEL_BACKITEMS_ALPHA, CAROUSEL_FRONTITEM_ALPHA, percentage );
        
    } else {    // Going the left.
        
        percentage = CGFloat(fabs( percentage ));

        CGRect newMainRect = LerpRect( mainRect, prevRect , percentage );
        CGRect newNextRect = LerpRect( nextRect, mainRect , percentage );
        CGRect newPrevRect = LerpRect( prevRect, nextRect , percentage );
        
        mainSelection.frame = newMainRect;
        prevSelection.frame = newPrevRect;
        nextSelection.frame = newNextRect;
        
        mainSelection.alpha = Lerp( CAROUSEL_FRONTITEM_ALPHA, CAROUSEL_BACKITEMS_ALPHA, percentage );
        nextSelection.alpha = Lerp( CAROUSEL_BACKITEMS_ALPHA, CAROUSEL_FRONTITEM_ALPHA, percentage );
    }
}

/*
 ========================
 touchesEnded
 ========================
 */
- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
    (void)(event);
    (void)(touches);
    
    if( hasDragged == CAROUSEL_DRAG_MOVE_FORWARD ) {
        [ self RotateForward ];
    } else if( hasDragged == CAROUSEL_DRAG_MOVE_BACKWARD ) {
        [ self RotateBackward ];
    } else {
        [ self RotateToOrigin ];
    }
}

/*
 ========================
 Init
 ========================
 */
- (void) Init {
    
    // Get the items At the new layout.
    idCarouselItem * cItem = [ carouselItems objectAtIndex: (NSUInteger)currentItem ];
    idCarouselItem * nItem = [ carouselItems objectAtIndex: (NSUInteger)nextItem ];
    idCarouselItem * pItem = [ carouselItems objectAtIndex: (NSUInteger)prevItem ];
    
    // Set the Main Selection Text.
    [ selectionLabel setText: cItem.title ];
    
    // Set the prev selection UIImage
    mainSelection.image = cItem.image;
    prevSelection.image = nItem.image;
    nextSelection.image = pItem.image;
}

/*
 ========================
 AddCarouselItem
 ========================
 */
- (void) AddCarouselItem:( NSString* )Image
						:( NSString* )Title
						:( int ) ItemHash {
    
    idCarouselItem * Item = [ idCarouselItem alloc ];
    
    // Initialize the Item with the data passed in.
    Item.image = [UIImage imageNamed: Image ];
    Item.title = Title;
    Item.hash = ItemHash;
    
    // Add the Item to our object array.
    [ carouselItems addObject: Item ];
    
    // Set the Cur Item as the one just added.
    currentItem = 0;    
    nextItem = 1;
    prevItem = (NSInteger)[ carouselItems count ] - 1;
    
}

/*
 ========================
 GetSelectedItem_Index
 ========================
 */
- (NSInteger) GetSelectedItem_Index {
    return currentItem;
}

/*
 ========================
 GetSelectedItem_Hash
 ========================
 */
- (int) GetSelectedItem_Hash {
    idCarouselItem * item = [ carouselItems objectAtIndex: (NSUInteger)currentItem ];
    
    return item.hash;
}

/*
 ========================
 MoveForward
 
Interface Builder Direct Access to moving the Carousel
 ========================
 */
- (IBAction) MoveForward {
    [ self RotateForward ];
}

/*
 ========================
 MoveBackward - 
 
 Interface Builder Direct Access to moving the Carousel
 ========================
 */
- (IBAction) MoveBackward {
    [ self RotateBackward ];
}

/*
 ========================
 RotateToOrigin
 ========================
 */
- (void) RotateToOrigin {
    
    // Animate back to the correct position.
    // Move the Current Selection to Next Selection
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
    [UIView setAnimationBeginsFromCurrentState:YES];
    
    // Move them into Place.
    mainSelection.frame = mainRect;
    prevSelection.frame = prevRect;
    nextSelection.frame = nextRect;
    
    mainSelection.alpha = 1.0f;
    nextSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    prevSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    
    [UIView commitAnimations];
}

/*
 ========================
 RotateForward
 ========================
 */
- (void)    RotateForward {
    
    if( prevSelection == nil || nextSelection == nil ) {
    
        // Flip book to the next selection.
        // increment the item indicies.
        currentItem++;
        nextItem++;
        prevItem++;
        
        // Check the Extents.
        if( currentItem >= (NSInteger)[ carouselItems count ] ) {
            currentItem = 0;
        }
        if( nextItem >= (NSInteger)[ carouselItems count ] ) {
            nextItem = 0;
        }
        if( prevItem >= (NSInteger)[ carouselItems count ] ) {
            prevItem = 0;
        }

        idCarouselItem * cItem = [ carouselItems objectAtIndex: (NSUInteger)currentItem ];
        
        // Set the Main Selection Text.
        [ selectionLabel setText: cItem.title ];
        
        // Set the prev selection UIImage
        mainSelection.image = cItem.image;
        
        [self sendActionsForControlEvents:UIControlEventValueChanged];
        
        return;
    }
    
    
    UIImageView * tempMain = mainSelection;
    UIImageView * tempPrev = prevSelection;
    UIImageView * tempNext = nextSelection;
    
    // Move the Current Selection to Next Selection
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
    [UIView setAnimationBeginsFromCurrentState:YES];
    
    // Move them into Place.
    mainSelection.frame = nextRect;
    prevSelection.frame = mainRect;
    nextSelection.frame = prevRect;
    
    // Darken the prev/next
    mainSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    prevSelection.alpha = CAROUSEL_FRONTITEM_ALPHA;
    nextSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    
    [UIView commitAnimations];
    
    
    // Swap out the ImageViews so that they are in the correct order.
    nextSelection = tempMain;
    mainSelection = tempPrev;
    prevSelection = tempNext;
    
    // Make sure they are in back of the main selection.
    [ self sendSubviewToBack: nextSelection ];
    [ self sendSubviewToBack: prevSelection ];
    
    // increment the item indicies.
    currentItem++;
    nextItem++;
    prevItem++;
    
    // Check the Extents.
    if( currentItem >= (NSInteger)[ carouselItems count ] ) {
        currentItem = 0;
    }
    if( nextItem >= (NSInteger)[ carouselItems count ] ) {
        nextItem = 0;
    }
    if( prevItem >= (NSInteger)[ carouselItems count ] ) {
        prevItem = 0;
    }
    
    // Get the items At the new layout.
    idCarouselItem * cItem = [ carouselItems objectAtIndex: (NSUInteger)currentItem ];
    idCarouselItem * nItem = [ carouselItems objectAtIndex: (NSUInteger)nextItem ];
    idCarouselItem * pItem = [ carouselItems objectAtIndex: (NSUInteger)prevItem ];
    
    // Set the Main Selection Text.
    [ selectionLabel setText: cItem.title ];
    
    // Set the prev selection UIImage
    mainSelection.image = cItem.image;
    prevSelection.image = nItem.image;
    nextSelection.image = pItem.image;
    
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

/*
 ========================
 RotateBackward
 ========================
 */
- (void)    RotateBackward {
    
    if( nextSelection == nil || prevSelection == nil ) {
        
        // increment the item indicies.
        currentItem--;
        nextItem--;
        prevItem--;
        
        // Check the Extents.
        if( currentItem < 0 ) {
            currentItem = (NSInteger)[ carouselItems count ] - 1;
        }
        if( nextItem < 0 ) {
            nextItem = (NSInteger)[ carouselItems count ] - 1;;
        }
        if( prevItem < 0 ) {
            prevItem = (NSInteger)[ carouselItems count ] - 1;;
        }
        
        idCarouselItem * cItem = [ carouselItems objectAtIndex: (NSUInteger)currentItem ];
        
        // Set the Main Selection Text.
        [ selectionLabel setText: cItem.title ];
        
        // Set the prev selection UIImage
        mainSelection.image = cItem.image;
        
        [self sendActionsForControlEvents:UIControlEventValueChanged];
        
        return;
    }
    
    UIImageView * tempMain = mainSelection;
    UIImageView * tempPrev = prevSelection;
    UIImageView * tempNext = nextSelection;
    
    // Move the Current Selection to Next Selection
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
    [UIView setAnimationBeginsFromCurrentState:YES];
    
    // Move them into Place.
    mainSelection.frame = prevRect;
    prevSelection.frame = nextRect;
    nextSelection.frame = mainRect;
    
    // Darken the prev/next
    mainSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    prevSelection.alpha = CAROUSEL_BACKITEMS_ALPHA;
    nextSelection.alpha = CAROUSEL_FRONTITEM_ALPHA;
    
    [UIView commitAnimations];
    
    // Swap out the ImageViews so that they are in the correct order.
    nextSelection = tempPrev;
    mainSelection = tempNext;
    prevSelection = tempMain;
    
    // Make sure they are in back of the main selection.
    [ self sendSubviewToBack: nextSelection ];
    [ self sendSubviewToBack: prevSelection ];
    
    // increment the item indicies.
    currentItem--;
    nextItem--;
    prevItem--;
    
    // Check the Extents.
    if( currentItem < 0 ) {
        currentItem = (NSInteger)[ carouselItems count ] - 1;
    }
    if( nextItem < 0 ) {
        nextItem = (NSInteger)[ carouselItems count ] - 1;;
    }
    if( prevItem < 0 ) {
        prevItem = (NSInteger)[ carouselItems count ] - 1;;
    }
    
    // Get the items At the new layout.
    idCarouselItem * cItem = [ carouselItems objectAtIndex: (NSUInteger) currentItem ];
    idCarouselItem * nItem = [ carouselItems objectAtIndex: (NSUInteger)nextItem ];
    idCarouselItem * pItem = [ carouselItems objectAtIndex: (NSUInteger)prevItem ];
    
    // Set the Main Selection Text.
    [ selectionLabel setText: cItem.title ];
    
    // Set the prev selection UIImage
    mainSelection.image = cItem.image;
    prevSelection.image = nItem.image;
    nextSelection.image = pItem.image;
    
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}

@end
