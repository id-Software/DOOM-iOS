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

#ifndef IDMOBILELIB_RENDER_CONTEXT_H
#define IDMOBILELIB_RENDER_CONTEXT_H

#if defined(__OBJC__) && defined(__cplusplus)

#import <QuartzCore/QuartzCore.h>
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>


class idRenderContext {
public:
    
                    idRenderContext();
    virtual         ~idRenderContext();
    
    void            Initialize( EAGLRenderingAPI apiVersion, EAGLSharegroup * sharegroup );
    void            InitFrameBuffer( CAEAGLLayer  * layer );
    
    void            Begin();
    void            End();
    
    void            SetActiveContext();
    void            SwapBuffers();
    
    void            SetContextWidth( int width );
    void            SetContextHeight( int height );
    
    int              GetContextWidth() const { return mWidth; }
    int              GetContextHeight() const { return mHeight; }
    EAGLSharegroup * GetShareGroup() { return [mEAGLContext sharegroup]; }
    
protected:
    
    GLuint          mViewRenderbuffer; 
	GLuint          mViewFramebuffer;
	GLuint          mDepthRenderbuffer;	
    
    EAGLContext     *mEAGLContext;
    
    int             mWidth;
    int             mHeight;
    
};

#endif 
    
#endif // _RENDER_CONTEXT_H
