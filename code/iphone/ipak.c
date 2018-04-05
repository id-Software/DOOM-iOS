/*
 *  ipak.c
 *  doom
 *
 *  Created by John Carmack on 4/9/09.
 *  Copyright 2009 Id Software. All rights reserved.
 *
 */
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

pkHeader_t	*pkHeader;
off_t		pkSize;

// images and wavs have writable state, so they need separate
// structs that also point to the source in the pak file
pkTexture_t	*pkTextures;
pkWav_t		*pkWavs;

void PK_LoadTexture( pkTexture_t *image );

/*
 ==================
 PK_Init
 
 ==================
 */
void PK_Init( const char *pakFileName ) {
	printf( "PK_Init( %s )\n", pakFileName );
	
	int fd = open( pakFileName, O_RDONLY );
	if ( fd == -1 ) {
		printf( "Couldn't open file\n" );
		assert( 0 );
	}

	struct stat s;
	fstat( fd, &s );
		
	pkSize = s.st_size;
	pkHeader = mmap( NULL, (size_t)pkSize, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0 );
	
	// mmap keeps the file internally, we can close our descriptor
	close( fd );
	
	if ( (int)pkHeader == -1 ) {
		printf( "mmap failed: %s\n", strerror( errno ) );
		assert( 0 );
	}
	
	if ( pkHeader->version != PKFILE_VERSION ) {
		printf( "bad pak file version: 0x%x != 0x%x\n", pkHeader->version, PKFILE_VERSION );
		assert( 0 );
	}
	
	// build the local image table
	pkTextures = malloc( sizeof( pkTextures[0] ) * pkHeader->textures.count );
	memset( pkTextures, 0, sizeof( pkTextures[0] ) * pkHeader->textures.count );
	for ( int i = 0 ; i < pkHeader->textures.count ; i++ ) {
		pkTextures[i].textureData = (pkTextureData_t *)( (byte *)pkHeader + pkHeader->textures.tableOfs + i * pkHeader->textures.structSize );
	}
	
	// build the local wav table
	int	startLoadingWavs = SysIphoneMicroseconds();
	pkWavs = malloc( sizeof( pkWavs[0] ) * pkHeader->wavs.count );
	memset( pkWavs, 0, sizeof( pkWavs[0] ) * pkHeader->wavs.count );
	for ( int i = 0 ; i < pkHeader->wavs.count ; i++ ) {
		pkWav_t *sfx = &pkWavs[i];
		sfx->wavData = (pkWavData_t *)( (byte *)pkHeader + pkHeader->wavs.tableOfs + i * pkHeader->wavs.structSize );
		// there is no harm in setting the OpenAl static buffer up for everything now
		alGenBuffers( 1, &sfx->alBufferNum );

		int	alFormat;
		if ( sfx->wavData->wavChannels == 1 ) {
			if ( sfx->wavData->wavChannelBytes == 1 ) {
				alFormat = AL_FORMAT_MONO8;
			} else {
				alFormat = AL_FORMAT_MONO16;
			}
		} else {
			if ( sfx->wavData->wavChannelBytes == 1 ) {
				alFormat = AL_FORMAT_STEREO8;
			} else {
				alFormat = AL_FORMAT_STEREO16;
			}
		}			
		alBufferData( sfx->alBufferNum, alFormat, (byte *)pkHeader + sfx->wavData->wavDataOfs
					   , sfx->wavData->wavChannels*sfx->wavData->wavChannelBytes*sfx->wavData->wavNumSamples
					 , sfx->wavData->wavRate );		
	}
	int	endLoadingWavs = SysIphoneMicroseconds();
	printf( "%i usec to load wavs\n", endLoadingWavs - startLoadingWavs );
	
	printf( "Mapped %lld bytes of %s at 0x%p\n", pkSize, pakFileName, (void*)pkHeader );
	printf( "%4i textures\n", pkHeader->textures.count );
	printf( "%4i wavs\n", pkHeader->wavs.count );
	printf( "%4i raws\n", pkHeader->raws.count );
#if 0
	// testing
	for ( int j = 0 ; j < 4 ; j++ ) {
		int startTime = Sys_Microseconds();
		int	sum = 0;
		for ( off_t i = 0 ; i < pkSize ; i+=16 ) {
			sum += ((byte *)pkHeader)[i];
		}
		int endTime = Sys_Microseconds();
		printf( "%5.1f mb/s page-in speed (%i)\n", (float)pkSize / (endTime - startTime ), endTime - startTime );
	}
	
	for ( int i = 0 ; i < pkHeader->numTextures ; i++ ) {
		printf( "-------------------------\n" );
		for ( int j = 0 ; j < 8 ; j++ ) {
			pkTexture_t *tex = &pkTextures[i];
			int start = Sys_Microseconds();
			PK_LoadTexture( tex );
			int middle = Sys_Microseconds();
			PK_StretchTexture( tex, 0, 0, 0, 0 );
			int middle2 = Sys_Microseconds();
			PK_StretchTexture( tex, 0, 0, 0, 0 );
			int end = Sys_Microseconds();
			printf( "%i usec load, %i usec first draw, %i usec second draw\n", 
				   middle - start, middle2 - middle, end - middle2 );
			
			glDeleteTextures( 1, &tex->glTexNum );
			tex->glTexNum = 0;
		}
	}
#endif	
}


/*
 ==================
 PK_LoadTexture
 
 ==================
 */
void PK_LoadTexture( pkTexture_t *tex ) {
	int startTime = SysIphoneMicroseconds();

	const pkTextureData_t *imd = tex->textureData;

	glGenTextures( 1, &tex->glTexNum );
	glBindTexture( GL_TEXTURE_2D, tex->glTexNum );
	
	// load the image directly from the mapped file
	typedef struct {
		int		internalFormat;
		int		externalFormat;
		int		type;
		int		bpp;
	} formatInfo_t;
	
	static formatInfo_t formatInfo[9] = {
		{ GL_RGB , GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 16 },
		{ GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 16 },
		{ GL_RGBA, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 16 },
		{ GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, 32 },
		{ GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 16 },
		{ GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, 0, 0, 4 },
		{ GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, 0, 0, 4 },
		{ GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, 0, 0, 2 },
		{ GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, 0, 0, 2 },
	};
	
	assert( imd->format < 9 );
	formatInfo_t *fi = &formatInfo[imd->format];
	
	unsigned char *s = (byte *)pkHeader + imd->picDataOfs;
	int	w = imd->uploadWidth;
	int h = imd->uploadHeight;
	// upload each mip level
	int l = 0;
	int	totalSize = 0;
	while( 1 ) {
		int	size = (w*h*fi->bpp)/8;
		if ( fi->type == 0 ) {
			if ( size < 32 ) {
				// minimum PVRTC size
				size = 32;
			}
			glCompressedTexImage2D( GL_TEXTURE_2D, l, fi->internalFormat, w, h, 0, 
									size, s );
		} else {
			glTexImage2D( GL_TEXTURE_2D, l, fi->internalFormat, w, h, 0, 
						  fi->externalFormat, fi->type, s );
		}
		GLCheckError( "texture upload" );

		totalSize += size;
		if ( ++l == imd->numLevels ) {
			break;
		}
		if ( w == 1 && h == 1 ) {
			break;
		}
		s += size;
		w >>= 1;
		if ( w == 0 ) {
			w = 1;
		}
		h >>= 1;
		if ( h == 0 ) {
			h = 1;
		}
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, imd->minFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, imd->magFilter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, imd->wrapS );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, imd->wrapT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f );
	
	int endTime = SysIphoneMicroseconds();
	
	printf( "%5.1f mb/s TexImage for %s\n",  (float)totalSize / 
		   ( endTime - startTime ), imd->name.name );
}

/*
 ==================
 PK_FindTexture
 
 Fully creates the gl texture before returning.
 ==================
 */
pkTexture_t *PK_FindTexture( const char *imageName ) {
	int	texIndex;
	pkTexture_t *texData = (pkTexture_t *)PK_FindType( imageName, &pkHeader->textures, &texIndex );
	if ( !texData ) {
		return NULL;
	}
	pkTexture_t *tex = pkTextures + texIndex;
	if ( tex->glTexNum == 0 ) {
		PK_LoadTexture( tex );
	}
	return tex;
}

/*
 ==================
 PK_FindWav
 
 ==================
 */
pkWav_t *PK_FindWav( const char *soundName ) {
	int	wavIndex;
	pkWavData_t *wavData = (pkWavData_t *)PK_FindType( soundName, &pkHeader->wavs, &wavIndex );
	if ( !wavData ) {
		return NULL;
	}
	pkWav_t *wav = pkWavs + wavIndex;
	
	// create the OpenAL buffer
	
	return wav;
}

/*
 ==================
 PK_FindRaw
 
 ==================
 */
const byte *PK_FindRaw( const char *rawName, int *len ) {
	pkRawData_t *raw = (pkRawData_t *)PK_FindType( rawName, &pkHeader->raws, NULL );
	if ( !raw ) {
		if ( len ) {
			*len = -1;
		}
		return NULL;
	}
	
	if ( len ) {
		*len = raw->rawDataLen;
	}
	return (byte *)pkHeader + raw->rawDataOfs;
}

/*
 ==================
 PK_HashName
 
 ==================
 */
int PK_HashName( const char *name, char canonical[MAX_PK_NAME] ) {
	int	o = 0;
	int	hash = 0;
	
	do {
		int c = name[o];
		if ( c == 0 ) {
			break;
		}
		// backslashes to forward slashes
		if ( c == '\\' ) {
			c = '/';
		}
		// to lowercase
		c = tolower( c );
		canonical[o++] = c;
		hash = (hash << 5) - hash + c;
	} while ( o < MAX_PK_NAME-1 );
	canonical[o] = 0;
	
	return hash;
}

/*
 ==================
 PK_FindType
 
 ==================
 */
const pkName_t *PK_FindType( const char *rawName, const pkType_t *type, int *indexOutput ) {
	char	canonicalName[MAX_PK_NAME];
	
	int	hash = PK_HashName( rawName, canonicalName );
	
	int hashChain = hash & (PK_HASH_CHAINS-1);
	
	int	typeIndex = type->hashChains[hashChain];
	while ( typeIndex != -1 ) {
		assert( typeIndex >= 0 && typeIndex < type->count );
		const pkName_t *name = (pkName_t *)((byte *)pkHeader + type->tableOfs + typeIndex * type->structSize );
		if ( name->nameHash == hash && !strcmp( canonicalName, name->name ) ) {
			// this is it
			if ( indexOutput ) {
				*indexOutput = typeIndex;
			}
			return name;
		}
		typeIndex = name->nextOnHashChain;
	}
	
	// not found
	if ( indexOutput ) {
		*indexOutput = -1;
	}
	return NULL;
}


/*
 ==================
 PK_BindTexture
 
 ==================
 */
void PK_BindTexture( pkTexture_t *tex ) {
	assert( tex->glTexNum );
	glBindTexture( GL_TEXTURE_2D, tex->glTexNum );
}

/*
 ==================
 PK_DrawTexture
 
 ==================
 */
void PK_DrawTexture( pkTexture_t *tex, int x, int y ) {
	PK_BindTexture( tex );

	int w = tex->textureData->srcWidth;
	int h = tex->textureData->srcHeight;
    
	glBegin( GL_QUADS );
	
	glTexCoord2f( 0.0f, 0.0f );	glVertex2i( x, y );
	glTexCoord2f( tex->textureData->maxS, 0.0f );	glVertex2i( x+w, y );
	glTexCoord2f( tex->textureData->maxS, tex->textureData->maxT );	glVertex2i( x+w, y+h );
	glTexCoord2f( 0.0f, tex->textureData->maxT );	glVertex2i( x, y+h );
	
	glEnd();
}

void PK_StretchTexture( pkTexture_t *tex, float x, float y, float w, float h ) {
	PK_BindTexture( tex );
	
	glBegin( GL_QUADS );
	
	glTexCoord2f( 0.0f, 0.0f );	glVertex2i( x, y );
	glTexCoord2f( tex->textureData->maxS, 0.0f );	glVertex2i( x+w, y );
	glTexCoord2f( tex->textureData->maxS, tex->textureData->maxT );	glVertex2i( x+w, y+h );
	glTexCoord2f( 0.0f, tex->textureData->maxT );	glVertex2i( x, y+h );
	
	glEnd();
}

