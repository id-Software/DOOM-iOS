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

#include "RenderContext.h"

/*
 =========================================
 idRenderContext::idRenderContext
 =========================================
 */
idRenderContext::idRenderContext() :
mViewRenderbuffer( 0 ), 
mViewFramebuffer( 0 ), 
mDepthRenderbuffer( 0 ),
mEAGLContext( NULL ) {
    
    mWidth = 0;
    mHeight = 0;
}

/*
 =========================================
 idRenderContext::Initialize
 =========================================
 */
void idRenderContext::Initialize( EAGLRenderingAPI apiVersion,  EAGLSharegroup * sharegroup  ) {
    
    // Allocate and initialize the Rendering context for this view.
    if( sharegroup == NULL ) {
        mEAGLContext = [[EAGLContext alloc] initWithAPI:apiVersion];
    } else {
        
        mEAGLContext = [[EAGLContext alloc] initWithAPI:apiVersion sharegroup: sharegroup];
    }
    
    assert( mEAGLContext );
	assert( [mEAGLContext API] == apiVersion );
    
    SetActiveContext();
}

/*
 =========================================
 idRenderContext::~idRenderContext
 =========================================
 */
idRenderContext::~idRenderContext() {
    
    glDeleteFramebuffersOES( 1, &mViewFramebuffer );
	mViewFramebuffer = 0;
	glDeleteRenderbuffersOES( 1, &mViewRenderbuffer );
	mViewRenderbuffer = 0;
	glDeleteRenderbuffersOES( 1, &mDepthRenderbuffer );
	mDepthRenderbuffer = 0;
    
    [ mEAGLContext release ];
}

/*
 =========================================
 idRenderContext::SetActiveContext
 =========================================
 */
void idRenderContext::SetActiveContext() {
    
    if ( ![EAGLContext setCurrentContext:mEAGLContext]) {
		assert( 0 );
		return;
	}
    
}

/*
 =========================================
 idRenderContext::SwapBuffers
 =========================================
 */
void idRenderContext::SwapBuffers() {
           
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, mViewRenderbuffer);
    
    // present the renderbuffer for display
    [mEAGLContext presentRenderbuffer:GL_RENDERBUFFER_OES];

}

/*
 =========================================
 idRenderContext::RenderBufferStorage
 =========================================
 */
void idRenderContext::InitFrameBuffer( CAEAGLLayer  * layer ) {
    
    
    glDeleteFramebuffersOES( 1, &mViewFramebuffer );
	mViewFramebuffer = 0;
	glDeleteRenderbuffersOES( 1, &mViewRenderbuffer );
	mViewRenderbuffer = 0;
	glDeleteRenderbuffersOES( 1, &mDepthRenderbuffer );
	mDepthRenderbuffer = 0;
    
    glGenFramebuffersOES(1, &mViewFramebuffer);
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, mViewFramebuffer);	
    
    glGenRenderbuffersOES(1, &mViewRenderbuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, mViewRenderbuffer);
    
    [mEAGLContext renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
    
    // the backing sizes should be the same as the screen
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &mWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &mHeight);
    
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, mViewRenderbuffer);
    
    glGenRenderbuffersOES(1, &mDepthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, mDepthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, mWidth, mHeight);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, mDepthRenderbuffer);
    
    // the framebuffer will stay constant
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, mViewRenderbuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, mViewFramebuffer);

    
    if ( glCheckFramebufferStatusOES( GL_FRAMEBUFFER_OES ) != GL_FRAMEBUFFER_COMPLETE_OES ) {
		assert( 0 );
    }

}

/*
 =========================================
 idRenderContext::Begin
 =========================================
 */
void idRenderContext::Begin() {
    
    SetActiveContext();
    
    // Clear the Buffer to begin for a new Frame.
    //GL_Clear( true, true, false );  
    
    // Set the Viewport for our Context.
   // GL_Viewport( 0, 0, mWidth, mHeight );
    
}

/*
 =========================================
 idRenderContext::End
 =========================================
 */
void idRenderContext::End() {
    
    SwapBuffers();
}
