/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

#ifndef _GL_INTERN_H
#define _GL_INTERN_H

typedef enum
{
  GLDT_UNREGISTERED,
  GLDT_BROKEN,
  GLDT_PATCH,
  GLDT_TEXTURE,
  GLDT_FLAT
} GLTexType;

typedef struct
{
  int index;
  int width,height;
  int leftoffset,topoffset;
  int tex_width,tex_height;
  int realtexwidth, realtexheight;
  int buffer_width,buffer_height;
  int buffer_size;
  GLuint glTexID[CR_LIMIT+MAXPLAYERS];
  GLTexType textype;
  boolean mipmap;
} GLTexture;

// JDC: moved these to header ---------------------------

#define MAP_COEFF 128.0f
#define MAP_SCALE (MAP_COEFF*(float)FRACUNIT)


typedef struct
	{
		float x1,x2;
		float z1,z2;
	} GLSeg;

#define GLDWF_TOP 1
#define GLDWF_M1S 2
#define GLDWF_M2S 3
#define GLDWF_BOT 4
#define GLDWF_SKY 5
#define GLDWF_SKYFLIP 6

typedef struct
	{
		GLSeg *glseg;
		float ytop,ybottom;
		float ul,ur,vt,vb;
		float light;
		float alpha;
		float skyymid;
		float skyyaw;
		GLTexture *gltexture;
		byte flag;
#ifdef IPHONE
		side_t	*side;
#endif
	} GLWall;

typedef struct
	{
		int sectornum;
		float light; // the lightlevel of the flat
		float uoffs,voffs; // the texture coordinates
		float z; // the z position of the flat (height)
		GLTexture *gltexture;
		boolean ceiling;
	} GLFlat;

typedef struct
	{
		int cm;
		float x,y,z;
		float vt,vb;
		float ul,ur;
		float x1,y1;
		float x2,y2;
		float light;
		fixed_t scale;
		GLTexture *gltexture;
		boolean shadow;
		boolean trans;
	} GLSprite;

typedef enum
	{
		GLDIT_NONE,
		GLDIT_WALL,
		GLDIT_FLAT,
		GLDIT_SPRITE
	} GLDrawItemType;

typedef struct
	{
		GLDrawItemType itemtype;
		int itemcount;
		int firstitemindex;
		byte rendermarker;
	} GLDrawItem;

typedef struct
	{
		GLWall *walls;
		int num_walls;
		int max_walls;
		GLFlat *flats;
		int num_flats;
		int max_flats;
		GLSprite *sprites;
		int num_sprites;
		int max_sprites;
		GLDrawItem *drawitems;
		int num_drawitems;
		int max_drawitems;
	} GLDrawInfo;

extern GLDrawInfo gld_drawinfo;

typedef struct
	{
		GLfloat x;
		GLfloat y;
		GLfloat z;
	} GLVertex;

typedef struct
	{
		GLfloat u;
		GLfloat v;
	} GLTexcoord;


/* GLLoopDef is the struct for one loop. A loop is a list of vertexes
 * for triangles, which is calculated by the gluTesselator in gld_PrecalculateSector
 * and in gld_PreprocessCarvedFlat
 */
typedef struct
	{
		GLenum mode; // GL_TRIANGLES, GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN
		int vertexcount; // number of vertexes in this loop
		int vertexindex; // index into vertex list
	} GLLoopDef;

// GLSector is the struct for a sector with a list of loops.

typedef struct
	{
		int loopcount; // number of loops for this sector
		GLLoopDef *loops; // the loops itself
	} GLSector;

extern GLSector *sectorloops;

typedef struct drawVert_s {		// JDC
	float	xyz[3];				// TODO: adjust MAP_SCALE, make shorts
	float	st[2];				// TODO: set texture matrix, make shorts
	unsigned char rgba[4];
} drawVert_t;
#define MAX_DRAW_VERTS	0x10000
extern drawVert_t	drawVerts[MAX_DRAW_VERTS];
extern int		numDrawVerts;

//--------------------------------------

extern int gld_max_texturesize;
extern const char *gl_tex_format_string;
extern int gl_tex_format;
extern int gl_tex_filter;
extern int gl_mipmap_filter;
extern int gl_texture_filter_anisotropic;
extern int gl_paletted_texture;
extern int gl_shared_texture_palette;
extern boolean use_mipmapping;
extern int transparent_pal_index;
extern unsigned char gld_palmap[256];
extern GLTexture *last_gltexture;
extern int last_cm;

//e6y: in some cases textures with a zero index (NO_TEXTURE) should be registered
GLTexture *gld_RegisterTexture(int texture_num, boolean mipmap, boolean force);
void gld_BindTexture(GLTexture *gltexture);
GLTexture *gld_RegisterPatch(int lump, int cm);
void gld_BindPatch(GLTexture *gltexture, int cm);
GLTexture *gld_RegisterFlat(int lump, boolean mipmap);
void gld_BindFlat(GLTexture *gltexture);
void gld_InitPalettedTextures(void);
int gld_GetTexDimension(int value);
void gld_SetTexturePalette(GLenum target);
void gld_Precache(void);

extern PFNGLCOLORTABLEEXTPROC gld_ColorTableEXT;

#endif // _GL_INTERN_H
