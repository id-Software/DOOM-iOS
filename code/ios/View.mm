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

#import "View.h"

/*
 ================================================
 idView 
 
 Standard Objective C UIView Class
 ================================================
 */

idView * gMainView; // global openGL View.

@implementation idView

/*
 ==================================
 idView::layerClass
 ==================================
 */
+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

/*
 ==================================
 idView::initWithCoder
 ==================================
 */
- (id) initWithCoder:(NSCoder *)aCoder{
    
    if ( ( self = [super initWithCoder:aCoder] ) ) {
        [self Initialize];
    }
    
    return self;
}

/*
 ==================================
 idView::initWithFrame
 ==================================
 */
- (id) initWithFrame:(CGRect)rect{
    if ( ( self = [super initWithFrame:rect] ) ) {
        [self Initialize];
    }
    return self;
}

/*
 ==================================
 idView::handleTouches
 ==================================
 */
- (void) handleTouches:(UIEvent*)event {
	(void)event;
}


/*
 ==================================
 idView::touchesBegan
 ==================================
*/
- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
	(void)touches;
	
	[self handleTouches:event];
}
 
/*
 ==================================
 idView::touchesMoved
 ==================================
*/ 
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	(void)touches;
	
	[self handleTouches:event];
}

/*
 ==================================
 idView::touchesEnded
 ==================================
*/ 
- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {

    (void)touches;
	
	[self handleTouches:event];
}

/*
 ==================================
 idView::touchesCancelled
 ==================================
 */
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    
    (void)touches;
	
	[self handleTouches:event];
}

/*
 ==================================
 idView::Initialize
 ==================================
 */
- (void) Initialize {
    
    self.multipleTouchEnabled = YES;
	gMainView = self;
    
    // Double the resolution on iPhone 4.   
    [self setContentScaleFactor:[UIScreen mainScreen].scale];
    
    // Get the layer for this view
	CAEAGLLayer *glLayer = (CAEAGLLayer *)self.layer;
	glLayer.opaque = TRUE;
	glLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                  [NSNumber numberWithBool:FALSE],	kEAGLDrawablePropertyRetainedBacking, 
                                  kEAGLColorFormatRGB565,
                                  kEAGLDrawablePropertyColorFormat, nil];
    
    // Create Our Render Context.
    mRenderContext.Initialize( kEAGLRenderingAPIOpenGLES1, NULL );
    
    // Create our Frame Buffers.    
    mRenderContext.InitFrameBuffer( glLayer );
}

/*
 ==================================
 idView::EndFrame
 ==================================
 */
- (void) EndFrame {
    
    mRenderContext.SwapBuffers();
}

@end
