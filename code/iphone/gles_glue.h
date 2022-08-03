/*
 
 Copyright (C) 2009 Id Software, Inc.
 
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


#ifndef __GLES_GLUE_H__
#define __GLES_GLUE_H__

#include <OpenGLES/ES1/gl.h>

#define GL_QUADS 888

#ifdef __cplusplus
extern "C" {
#endif

void GLCheckError( const char *message );

void glBegin( GLenum prim );
void glVertex2f( GLfloat x, GLfloat y );
void glVertex3f( float x, float y, float z );
void glVertex3fv( GLfloat *xyz );
void glVertex2i( GLint x, GLint y );
void glColor4ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a );
void glColor4ubv( GLubyte *rgba );
void glColor3f( GLfloat r, GLfloat g, GLfloat b );
void glColor4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void glColor4fv( GLfloat *rgba );
void glTexCoord2i( GLint s, GLint t );
void glTexCoord2f( GLfloat s, GLfloat t );
void glEnd( void );

void SetImmediateModeGLVertexArrays( void );

#ifdef __cplusplus
}
#endif

#endif
