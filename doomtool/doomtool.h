/*
 *  ipak.h
 *  General purpose data file management intended to be used
 *  as a read-only memory mapped file to play nice with iPhone OS's
 *  non-swapping and variable memory management.
 *
 *  Created by John Carmack on 4/9/09.
 *  Copyright 2009 id Software. All rights reserved.
 *
 */

//============================================================
//
// In-file structures
//
// These stuctures are in the mapped data file, and shared
// between the app and utility.
//
// Type headers are stored separately from the bulk data to minimize the
// number of active pages.
//
// The full hash of the name is stored in nameHash, and nameHash&(PK_HASH_BUCKETS-1) is
// used to chain structures of a particular type together.
//
//============================================================

#define MAX_PK_NAME	64
typedef struct {
	int		nameHash;			// PK_HashName( name )
	int		nextOnHashChain;	// -1 = end of chain
	char	name[MAX_PK_NAME];	// in canonical form: backslashes to slashes and lowercase
} pkName_t;

#define PK_HASH_CHAINS	256
typedef struct {
	int		tableOfs;			// // &firstStruct = (byte *)dfHeader + tableOfs
	int		count;
	int		structSize;			// sizeof( pkWavData_t ), etc
	int		hashChains[PK_HASH_CHAINS];	// -1 = end of chain
} pkType_t;

// dfWavData holds everything necessary to fully create an OpenAL sample buffer
typedef struct {
	pkName_t	name;
	int		wavDataOfs;
	int		wavChannels;		// 1 or 2
	int		wavChannelBytes;	// 1 or 2
	int		wavRate;			// 22050, etc
	int		wavNumSamples;		// each sample holds all the channels
	// we may want looping information here later
} pkWavData_t;

// iPhone does not natively support palettized textures, but we
// might conceivably want to support luminance and intensity textures
// in the future.
typedef enum {
	TF_565,
	TF_5551,
	TF_4444,
	TF_8888,
	TF_LA,
	TF_PVR4,
	TF_PVR4A,
	TF_PVR2,
	TF_PVR2A,
} textureFormat_t;

// dfImageData_t holds everything necessary to fully create an OpenGL texture object
typedef struct {
	pkName_t	name;
	int		picDataOfs;		// the raw bits to pass to gl, mipmaps appended
	// for PVR formats, the minimum size of each level is 32 bytes
	
	int		format;
	int		uploadWidth;
	int		uploadHeight;
	int		numLevels;		// 1 for non mipmapped, otherwise log2( largest dimension )
	
	// glTexParameters
	int		wrapS;
	int		wrapT;
	int		minFilter;
	int		magFilter;
	int		aniso;
	
	// The upload sizes can be larger than the source sizes for
	// non power of two sources, or for non square sources in the
	// case of PVR compression.
	int		srcWidth;
	int		srcHeight;
	
	float	maxS;			// srcWidth / uploadWidth
	float	maxT;	
	
	// Track the outlines of up to two boxes of non-transparent pixels
	// to allow optimized drawing of sprites with large empty areas.
	// The reason for two boxes is that the common lights have something
	// at the top and something at the bottom, with nothing inbetween.
	// These are inclusive bounds of the rows / columns in
	// uploadWidth / uploadHeight with non-0 alpha 
	int		numBounds;
	int		bounds[2][2][2];	
} pkTextureData_t;

typedef struct {
	pkName_t	name;
	int		rawDataOfs;		// (byte *)pkHeader + dataOfs
	int		rawDataLen;		// there will always be a 0 byte appended to terminate strings
	// that is not counted in this length
} pkRawData_t;

#define PKFILE_VERSION	0x12340002
typedef struct {
	int	version;
	
	pkType_t	textures;
	pkType_t	wavs;
	pkType_t	raws;
} pkHeader_t;


//============================================================
//
// In-memory, writable structures
//
//============================================================

typedef struct {
	unsigned	glTexNum;
	const pkTextureData_t	*textureData;
	// we will need to add LRU links if texture caching is needed
} pkTexture_t;

typedef struct {
	unsigned		alBufferNum;		// created with the staticBuffer extension directly in the mapped memory
	const pkWavData_t	*wavData;
} pkWav_t;

void			PK_Init( const char *pakFileName );
const pkName_t *PK_FindType( const char *rawName, const pkType_t *type, int *index );
const byte *	PK_FindRaw( const char *rawName, int *len );	// len can be NULL if you don't need it
pkTexture_t *	PK_FindTexture( const char *imageName );
pkWav_t *		PK_FindWav( const char *soundName );

// The name will be converted to canonical name (backslashes converted to slashes and lowercase)
// before generating a hash.
int				PK_HashName( const char *name, char canonical[MAX_PK_NAME] );

void			PK_BindTexture( pkTexture_t *tex );
void			PK_DrawTexture( pkTexture_t *tex, int x, int y );
void			PK_StretchTexture( pkTexture_t *tex, float x, float y, float w, float h );

extern pkHeader_t *	pkHeader;
extern int			pkSize;

// images and wavs have writable state, so they need separate
// structs that also point to the source in the pak file
extern pkTexture_t *pkTextures;
extern pkWav_t *	pkWavs;

