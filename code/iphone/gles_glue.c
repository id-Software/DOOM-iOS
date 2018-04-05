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


#include "doomiphone.h"

//int registration_sequence;

//#include "iphone_gl.h"

void R_Draw_Blend( int x, int y, int w, int h, color4_t c ) {
    
    x *= ((float)displaywidth) / 480.0f;
    y *= ((float)displayheight) / 320.0f;
    w *= ((float)displaywidth) / 480.0f;
    h *= ((float)displayheight) / 320.0f;
    
	glDisable( GL_TEXTURE_2D );	
	glColor4ubv( c );
	
	glBegin( GL_QUADS );
	
	glVertex2i( x, y );
	glVertex2i( x+w, y );
	glVertex2i( x+w, y+h );
	glVertex2i( x, y+h );
	
	glEnd();
	
	glColor3f( 1, 1, 1 );
	glEnable( GL_TEXTURE_2D );
}


void R_Draw_Fill( int x, int y, int w, int h, color3_t c ) {
	// as of 2.2 OS, doing a clear with a small scissor rect is MUCH slower
	// than drawing geometry
	color4_t	c4;
	c4[0] = c[0];
	c4[1] = c[1];
	c4[2] = c[2];
	c4[3] = 255;
	R_Draw_Blend( x, y, w, h, c4 );
}

void GLCheckError(const char *message) {
	GLint err = glGetError();
	if ( err != GL_NO_ERROR ) {
		printf( "GL ERROR %d from %s\n", err, message );
	}
}

unsigned int QGLBeginStarted = 0;

struct Vertex {
	float xyz[3];
	float st[2];
#ifdef VERTEX_COLOR	
	GLubyte c[4];
#endif	
};

#define MAX_VERTS 16384

typedef struct Vertex Vertex;
Vertex immediate[ MAX_VERTS ];
Vertex vab;
short quad_indexes[MAX_VERTS * 3 / 2 ];
int curr_vertex;
GLenum curr_prim;

void		SetImmediateModeGLVertexArrays( void ) {
	glVertexPointer( 3, GL_FLOAT, sizeof( Vertex ), immediate[ 0 ].xyz );
	glEnableClientState( GL_VERTEX_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, sizeof( Vertex ), immediate[ 0 ].st );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
#ifdef VERTEX_COLOR	
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( Vertex ), immediate[ 0 ].c );
	glEnableClientState( GL_COLOR_ARRAY );
#endif
}

void		InitImmediateModeGL( void ) {
	for ( int i = 0; i < MAX_VERTS * 3 / 2; i+=6 ) {
		int q = i / 6 * 4;
		quad_indexes[ i + 0 ] = q + 0;
		quad_indexes[ i + 1 ] = q + 1;
		quad_indexes[ i + 2 ] = q + 2;
		
		quad_indexes[ i + 3 ] = q + 0;
		quad_indexes[ i + 4 ] = q + 2;
		quad_indexes[ i + 5 ] = q + 3;
	}

	SetImmediateModeGLVertexArrays();
}

void glBegin( GLenum prim ) {
	curr_vertex = 0;
	curr_prim = prim;
}

void glVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
	assert( curr_vertex < MAX_VERTS );
	vab.xyz[ 0 ] = x;
	vab.xyz[ 1 ] = y;
	vab.xyz[ 2 ] = z;
	immediate[ curr_vertex ] = vab;
	curr_vertex++;
}
void glVertex3fv( GLfloat *xyz ) {
	assert( curr_vertex < MAX_VERTS );
	vab.xyz[ 0 ] = xyz[0];
	vab.xyz[ 1 ] = xyz[1];
	vab.xyz[ 2 ] = xyz[2];
	immediate[ curr_vertex ] = vab;
	curr_vertex++;
}
void glVertex2f( GLfloat x, GLfloat y ) {
	assert( curr_vertex < MAX_VERTS );
	vab.xyz[ 0 ] = (float)x;
	vab.xyz[ 1 ] = (float)y;
	vab.xyz[ 2 ] = 0.0f;
	immediate[ curr_vertex ] = vab;
	curr_vertex++;
}
void glVertex2i( GLint x, GLint y ) {
	assert( curr_vertex < MAX_VERTS );
	vab.xyz[ 0 ] = (float)x;
	vab.xyz[ 1 ] = (float)y;
	vab.xyz[ 2 ] = 0.0f;
	immediate[ curr_vertex ] = vab;
	curr_vertex++;
}

#ifdef VERTEX_COLOR
void glColor4ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a ) {
	vab.c[ 0 ] = r;
	vab.c[ 1 ] = g;
	vab.c[ 2 ] = b;
	vab.c[ 3 ] = a;
}
void glColor4ubv( GLubyte *rgba ) {
	vab.c[ 0 ] = rgba[0];
	vab.c[ 1 ] = rgba[1];
	vab.c[ 2 ] = rgba[2];
	vab.c[ 3 ] = rgba[3];
}
void glColor4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a ) {
	vab.c[ 0 ] = (GLubyte) ( r * 255 );
	vab.c[ 1 ] = (GLubyte) ( g * 255 );
	vab.c[ 2 ] = (GLubyte) ( b * 255 );
	vab.c[ 3 ] = (GLubyte) ( a * 255 );
}
void glColor4fv( GLfloat *rgba ) {
	vab.c[ 0 ] = (GLubyte) ( rgba[0] * 255 );
	vab.c[ 1 ] = (GLubyte) ( rgba[1] * 255 );
	vab.c[ 2 ] = (GLubyte) ( rgba[2] * 255 );
	vab.c[ 3 ] = (GLubyte) ( rgba[3] * 255 );
}
void glColor3f( GLfloat r, GLfloat g, GLfloat b ) {
	vab.c[ 0 ] = (GLubyte) ( r * 255 );
	vab.c[ 1 ] = (GLubyte) ( g * 255 );
	vab.c[ 2 ] = (GLubyte) ( b * 255 );
	vab.c[ 3 ] = 255;
}
#endif

void glTexCoord2i( GLint s, GLint t ) {
	vab.st[ 0 ] = (float)s;
	vab.st[ 1 ] = (float)t;
}
void glTexCoord2f( GLfloat s, GLfloat t ) {
	vab.st[ 0 ] = s;
	vab.st[ 1 ] = t;
}
void glTexCoord2fv( GLfloat *st ) {
	vab.st[ 0 ] = st[0];
	vab.st[ 1 ] = st[1];
}

void glEnd( void ) {
#if 0
	glVertexPointer( 3, GL_FLOAT, sizeof( Vertex ), immediate[ 0 ].xyz );
	glTexCoordPointer( 2, GL_FLOAT, sizeof( Vertex ), immediate[ 0 ].st );
	glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( Vertex ), immediate[ 0 ].c );
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
#endif	
	if ( curr_prim == GL_QUADS ) {
		glDrawElements( GL_TRIANGLES, curr_vertex / 4 * 6, GL_UNSIGNED_SHORT, quad_indexes );
		GLCheckError("DrawElements");
	} else {
		glDrawArrays( curr_prim, 0, curr_vertex );
		GLCheckError("DrawArrays");
	}
	curr_vertex = 0;
	curr_prim = 0;
}

void landscapeViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
	y = 0;	// !@#
    /* JDS proper fix for landscape orientation */
	glViewport( y, x, width, height );
}

void landscapeScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
	y = 0;	// !@#
	glScissor( y, x, height, width );
}

