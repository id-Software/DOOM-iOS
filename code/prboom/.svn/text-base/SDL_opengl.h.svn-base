/*
 *  SDL_opengl.h
 *  doom
 *
 *  Created by John Carmack on 4/13/09.
 *  Copyright 2009 idSoftware. All rights reserved.
 *
 * iPhone glue to get the prBoom code compiling
 * Replaces SDL_opengl.h
 */
#ifndef __SDL_OPENGL_H__
#define __SDL_OPENGL_H__

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>

#define GLAPIENTRY

// this needs to be added before each projection matrix
int iphoneRotateForLandscape();

// no colorTable in ES
typedef void (* PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);

static GLubyte *gluErrorString( int err ) { return (GLubyte *)"GLU error"; }
static void *SDL_GL_GetProcAddress( const char *proc ) { return 0; }
static void SDL_GL_SwapBuffers() {}

// we need to emulate immediate mode gl for ES
void glBegin( GLenum prim );

void glVertex3f( GLfloat x, GLfloat y, GLfloat z );
void glVertex3fv( GLfloat *xyz );
void glVertex2f( GLfloat x, GLfloat y );
void glVertex2i( GLint x, GLint y );

void glTexCoord2i( GLint s, GLint t );
void glTexCoord2f( GLfloat s, GLfloat t );
void glTexCoord2fv( GLfloat *st );

void glEnd();

// Doom just uses state color for all draw calls, setting it once
// before a glBegin, rather than setting it each vertex, so we
// don't need to emulate the color functions.
//#defne VERTEX_COLOR
#ifdef VERTEX_COLOR
void glColor4ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a );
void glColor4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a );
void glColor4fv( GLfloat *rgba );
void glColor3f( GLfloat r, GLfloat g, GLfloat b );
#endif

// GLES only defines glColor4ub and glColor4f, so define the others in terms of that
#define glColor4fv(x) glColor4f(x[0],x[1],x[2],x[3])
#define glColor4ubv(x) glColor4ub(x[0],x[1],x[2],x[3])
#define glColor3f(r,g,b) glColor4f(r,g,b,1)


// The width and height need to be flipped for iPhone landscape mode,
// so redefine these functions to something that can do the work behind
// the scenes.
void landscapeViewport( GLint x, GLint y, GLsizei width, GLsizei height );
void landscapeScissor( GLint x, GLint y, GLsizei width, GLsizei height );
#define glViewport	landscapeViewport
#define glScissor	landscapeScissor

// ES made matching fixed and floating versions of some functions
#define glClearDepth glClearDepthf
#define glOrtho glOrthof
#define glFogi glFogx

// no GLdouble in ES, but needed for glu tesselator
typedef double GLdouble;

// ES doesn't have the messy clamp-to-half-border mode
#define GL_CLAMP GL_CLAMP_TO_EDGE

// this is the internal format used by the prBoom gl code
// ES doesn't allow format conversions between external and internal,
// so we need to manually convert to 5551 before doing glTexSubImage
#define GL_RGB5_A1	GL_RGBA
#define GL_RGBA8	GL_RGBA
#define GL_RGBA4	GL_RGBA
#define GL_RGBA2	GL_RGBA

// not available in ES, so prBoom's skies must be implemeted differently
static void glTexGenfv( int a, int b, void *c ) { };
static void glTexGenf( int a, int b, int c ) { };

// texGen enums not present in ES
#define GL_S					0x2000
#define GL_T					0x2001
#define GL_R					0x2002
#define GL_Q					0x2003

#define GL_OBJECT_LINEAR			0x2401
#define GL_OBJECT_PLANE				0x2501
#define GL_EYE_LINEAR				0x2400
#define GL_EYE_PLANE				0x2502

#define GL_TEXTURE_GEN_MODE			0x2500

#define GL_TEXTURE_GEN_S			0x0C60
#define GL_TEXTURE_GEN_T			0x0C61
#define GL_TEXTURE_GEN_R			0x0C62
#define GL_TEXTURE_GEN_Q			0x0C63

// other extensions not present in ES
// Whlle the iPhone exports the extension for paletted
// textures, it isn't actually supported in hardware, so
// they are expanded internally on glTexImage2D, making their
// use completely counterproductive.
#define GL_SHARED_TEXTURE_PALETTE_EXT     0x81FB
#define GL_COLOR_INDEX				0x1900
#define GL_COLOR_INDEX8_EXT               0x80E5

//=========================== 
// all this for the glu tesselator, used by prBoom to make drawable sector geometry
//===========================

#include "../libtess/tess.h"

/* TessCallback */
#define GLU_TESS_BEGIN                     100100
#define GLU_BEGIN                          100100
#define GLU_TESS_VERTEX                    100101
#define GLU_VERTEX                         100101
#define GLU_TESS_END                       100102
#define GLU_END                            100102
#define GLU_TESS_ERROR                     100103
#define GLU_TESS_EDGE_FLAG                 100104
#define GLU_EDGE_FLAG                      100104
#define GLU_TESS_COMBINE                   100105
#define GLU_TESS_BEGIN_DATA                100106
#define GLU_TESS_VERTEX_DATA               100107
#define GLU_TESS_END_DATA                  100108
#define GLU_TESS_ERROR_DATA                100109
#define GLU_TESS_EDGE_FLAG_DATA            100110
#define GLU_TESS_COMBINE_DATA              100111


/* TessContour */
#define GLU_CW                             100120
#define GLU_CCW                            100121
#define GLU_INTERIOR                       100122
#define GLU_EXTERIOR                       100123
#define GLU_UNKNOWN                        100124

/* TessProperty */
#define GLU_TESS_WINDING_RULE              100140
#define GLU_TESS_BOUNDARY_ONLY             100141
#define GLU_TESS_TOLERANCE                 100142

/* TessError */
#define GLU_TESS_ERROR1                    100151
#define GLU_TESS_ERROR2                    100152
#define GLU_TESS_ERROR3                    100153
#define GLU_TESS_ERROR4                    100154
#define GLU_TESS_ERROR5                    100155
#define GLU_TESS_ERROR6                    100156
#define GLU_TESS_ERROR7                    100157
#define GLU_TESS_ERROR8                    100158
#define GLU_TESS_MISSING_BEGIN_POLYGON     100151
#define GLU_TESS_MISSING_BEGIN_CONTOUR     100152
#define GLU_TESS_MISSING_END_POLYGON       100153
#define GLU_TESS_MISSING_END_CONTOUR       100154
#define GLU_TESS_COORD_TOO_LARGE           100155
#define GLU_TESS_NEED_COMBINE_CALLBACK     100156

/* TessWinding */
#define GLU_TESS_WINDING_ODD               100130
#define GLU_TESS_WINDING_NONZERO           100131
#define GLU_TESS_WINDING_POSITIVE          100132
#define GLU_TESS_WINDING_NEGATIVE          100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO       100134

/* ErrorCode */
#define GLU_INVALID_ENUM                   100900
#define GLU_INVALID_VALUE                  100901
#define GLU_OUT_OF_MEMORY                  100902
#define GLU_INCOMPATIBLE_GL_VERSION        100903
#define GLU_INVALID_OPERATION              100904

#define GLAPI
#define GLAPIENTRYP

typedef struct GLUtesselator GLUtesselator;
typedef GLUtesselator GLUtesselatorObj;
typedef GLUtesselator GLUtriangulatorObj;

#define GLU_TESS_MAX_COORD 1.0e150

/* Internal convenience typedefs */
typedef void (GLAPIENTRYP _GLUfuncptr)();

GLAPI void GLAPIENTRY gluTessBeginContour (GLUtesselator* tess);
GLAPI void GLAPIENTRY gluTessBeginPolygon (GLUtesselator* tess, GLvoid* data);
GLAPI void GLAPIENTRY gluTessCallback (GLUtesselator* tess, GLenum which, _GLUfuncptr CallBackFunc);
GLAPI void GLAPIENTRY gluTessEndContour (GLUtesselator* tess);
GLAPI void GLAPIENTRY gluTessEndPolygon (GLUtesselator* tess);
GLAPI void GLAPIENTRY gluTessNormal (GLUtesselator* tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
GLAPI void GLAPIENTRY gluTessProperty (GLUtesselator* tess, GLenum which, GLdouble data);
GLAPI void GLAPIENTRY gluTessVertex (GLUtesselator* tess, GLdouble *location, GLvoid* data);

GLUtesselator * GLAPIENTRY gluNewTess( void );
void GLAPIENTRY gluDeleteTess( GLUtesselator *tess );

#endif


