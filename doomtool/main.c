#include <libc.h>
#include <assert.h>
#include <ctype.h>

/*

 store sprites in an atlas, or as discrete textures?
 
 store audio as 16 bit for direct mmap access, or 4 bit adpcm with dynamic decompress?
 
 render without a depth buffer?  NO: it wold defeat the deferred rendering ability
 
 optional script file with extra parameters


 youtube script
 --------------
 ipod music
 music icon
 landscape orientation
 skill level
 start game on any episode / map combination
 controls
 doors opening
 shooting
 damage indicators
 leave game at any time 
 custom controls 
 items in the world (ammo, health, treasure, weapons, keys)
 changing weapons
 secret doors
 finishing the level
 awards
 
 
 
 TASKS:
 Web page time check
 Data download
 Hardware mp3 playback, remove tremor
 Independent volume adjustment
 Broadcast packet tests
 Good console
 web media type for game launch and download
 Instrumented play data recording
 
 */

#include <dirent.h>
#include <OpenGL/gl.h>		// for repeat and filter enums
#include <OpenGL/glext.h>
typedef unsigned char byte;

#include "doomtool.h" 

const char *assetDirectory = "/Volumes/Work/idMobileDepot/Archive/DoomClassicDepot/assets";
const char *outputFile = "/Volumes/Work/idMobileDepot/Archive/DoomClassicDepot/base.iPack";
const char *parmFile = "/Volumes/Work/idMobileDepot/Archive/DoomClassicDepot/base.parm";

pkHeader_t	buildHeader;
FILE		*pakFile;
#define MAX_IMAGE_TABLE	10000
pkTextureData_t	buildTextureTable[MAX_IMAGE_TABLE];
#define MAX_WAV_TABLE	10000
pkWavData_t		buildWavTable[MAX_WAV_TABLE];
#define MAX_RAW_TABLE	10000
pkRawData_t		buildRawTable[MAX_RAW_TABLE];

// the doom extractor tool writes this out for alpha texels
#define DOOM_ALPHA_TEXEL 0xff00ffff

// the parm file is parsed for modifiers to specify image formats, etc
#define MAX_ARGV	16
typedef struct {
	int		argc;
	char	*argv[MAX_ARGV];	// argv[0] should be a filename local to the asset base
} parmLine_t;

#define MAX_PARM_LINES	10000
parmLine_t	parmLines[MAX_PARM_LINES];
int			numParmLines;

void Error( const char *fmt,  ... ) {
	va_list argptr;
	va_start( argptr, fmt );
	
	vprintf( fmt, argptr );
	exit( 1 );
}

int FileLength( FILE *f ) {
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	return len;
}

//====================================================================

const byte *iff_pdata;
const byte *iff_end;
const byte *iff_last_chunk;
const byte *iff_data;
int	iff_chunk_len;


short Wav_GetLittleShort( void )
{
	short val = 0;
	
	val = *iff_pdata;
	val += (*(iff_pdata + 1) << 8);
	
	iff_pdata += 2;
	
	return val;
}


int Wav_GetLittleLong( void )
{
	int val = 0;
	
	val =  *iff_pdata;
	val += (*(iff_pdata + 1) << 8);
	val += (*(iff_pdata + 2) << 16);
	val += (*(iff_pdata + 3) << 24);
	
	iff_pdata += 4;
	
	return val;
}


void Wav_FindNextChunk( const char *name )
{
	while( 1 )
	{
		iff_pdata = iff_last_chunk;
		
		if( iff_pdata >= iff_end )
		{
			// Didn't find the chunk
			iff_pdata = NULL;
			return;
		}
		
		iff_pdata += 4;
		iff_chunk_len = Wav_GetLittleLong();
		if( iff_chunk_len < 0 )
		{
			iff_pdata = NULL;
			return;
		}
		
		iff_pdata -= 8;
		iff_last_chunk = iff_pdata + 8 + ((iff_chunk_len + 1) & ~1);
		if( ! strncasecmp((const char *)iff_pdata, name, 4) )
		{
			return;
		}
	}
}


void Wav_FindChunk( const char *name )
{
	iff_last_chunk = iff_data;
	
	Wav_FindNextChunk( name );
}

/*
 ========================
 AddWAV
 
 ========================
 */
void AddWAV( const char *localName, const byte *data, int wavlength ) {
	assert( buildHeader.wavs.count < MAX_WAV_TABLE );
	pkWavData_t *wav = &buildWavTable[buildHeader.wavs.count++];
	
	iff_data = data;
	iff_end = data + wavlength;
	
	// look for RIFF signature
	Wav_FindChunk( "RIFF" );
	if( ! (iff_pdata && ! strncasecmp( (const char *)iff_pdata + 8, "WAVE", 4 ) ) ) {
		Error( "[LoadWavInfo]: Missing RIFF/WAVE chunks (%s)\n", localName );
	}
	
	// Get "fmt " chunk
	iff_data = iff_pdata + 12;
	
	Wav_FindChunk( "fmt " );
	if( ! iff_pdata ) {
		Error( "[LoadWavInfo]: Missing fmt chunk (%s)\n", localName );
	}
	
	iff_pdata += 8;
	
	if( Wav_GetLittleShort() != 1 ) {
		Error( "[LoadWavInfo]: Microsoft PCM format only (%s)\n", localName );
	}
	
	int channels = Wav_GetLittleShort();
	int sample_rate = Wav_GetLittleLong();
	
	iff_pdata += 4;
	
	// bytes per sample, which includes all channels
	// 16 bit stereo = 4 bytes per sample
	int sample_size = Wav_GetLittleShort();
	int	channelBytes = sample_size / channels;
	
	if ( channelBytes != 1 && channelBytes != 2 ) {
		Error( "[LoadWavInfo]: only 8 and 16 bit WAV files supported (%s)\n", localName );
	}
	
	iff_pdata += 2;
	
	// Find data chunk
	Wav_FindChunk( "data" );
	if( ! iff_pdata ) {
		Error( "[LoadWavInfo]: missing 'data' chunk (%s)\n", localName );
	}
	
	iff_pdata += 4;
	int numSamples = Wav_GetLittleLong() / sample_size;
	
	if( numSamples <= 0 ) {
		Error( "[LoadWavInfo]: file with 0 samples (%s)\n", localName );
	}
	
	// as of iphone OS 2.2.1, 8 bit samples cause audible pops at the beginning and end, so
	// convert them to 16 bit here
	const void *samples = data + (iff_pdata - data);
#if 0	
	if ( channelBytes == 1 ) {		
		int	numChannelSamples = numSamples * channels;
		channelBytes = 2;
		sample_size = channelBytes * channels;
		short *newSamples = alloca( numChannelSamples * sample_size );
		for ( int i = 0; i < numChannelSamples ; i++ ) {
			newSamples[i] = ((short)((const byte *)samples)[i] - 128) * 256;
		}
		samples = newSamples;
	}
#endif	
	// write out the raw data
	strcpy( wav->name.name, localName );
	wav->wavDataOfs = ftell( pakFile );
	fwrite( samples, numSamples, sample_size, pakFile );
	wav->wavChannels = channels;
	wav->wavChannelBytes = channelBytes;
	wav->wavRate = sample_rate;
	wav->wavNumSamples = numSamples;
}


/*
 ================================================================================================
 
 Bitmap Loading (.bmp)
 
 ================================================================================================
 */

typedef struct {
	char id[2];
	unsigned int fileSize;
	unsigned int reserved0;
	unsigned int bitmapDataOffset;
	unsigned int bitmapHeaderSize;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned int compression;
	unsigned int bitmapDataSize;
	unsigned int hRes;
	unsigned int vRes;
	unsigned int colors;
	unsigned int importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

/*
 ========================
 LoadBMP
 ========================
 */
static void LoadBMP( const char *name, byte **pic, int *width, int *height ) {
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	BMPHeader_t bmpHeader;
	byte		*bmpRGBA;
	
	*pic = NULL;
	
	//
	// load the file
	//
	FILE *f = fopen( name, "rb" );
	if ( !f ) {
		Error( "Can't open '%s'\n", name );
	}
	length = FileLength( f );
	buffer = malloc( length );
	fread( buffer, 1, length, f );
	fclose( f );

	buf_p = buffer;
	
	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.reserved0 = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.bitmapDataOffset = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.width = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.height = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.planes = * ( short * ) buf_p;
	buf_p += 2;
	bmpHeader.bitsPerPixel = * ( short * ) buf_p;
	buf_p += 2;
	bmpHeader.compression = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.bitmapDataSize = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.hRes = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.vRes = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.colors = * ( int * ) buf_p;
	buf_p += 4;
	bmpHeader.importantColors = * ( int * ) buf_p;
	buf_p += 4;
	
	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );
	
	if ( bmpHeader.bitsPerPixel == 8 ) {
		buf_p += 1024;
	}
	
	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) {
		Error( "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.fileSize != length ) {
		Error( "LoadBMP: header size does not match file size (%d vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if ( bmpHeader.compression != 0 ) {
		Error( "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.bitsPerPixel < 8 ) {
		Error( "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}
	
	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if ( rows < 0 ) {
		rows = -rows;
	}
	numPixels = columns * rows;
	
	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}
	
	bmpRGBA = (byte *)malloc( numPixels * 4 );
	*pic = bmpRGBA;
		
	byte *rowStart = buf_p;
	for ( row = rows-1; row >= 0; row-- ) {
		pixbuf = bmpRGBA + row*columns*4;
		buf_p = rowStart;
		for ( column = 0; column < columns; column++ ) {
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;
			
			switch ( bmpHeader.bitsPerPixel ) {
				case 8:
					palIndex = *buf_p++;
					*pixbuf++ = bmpHeader.palette[palIndex][0];
					*pixbuf++ = bmpHeader.palette[palIndex][1];
					*pixbuf++ = bmpHeader.palette[palIndex][2];
					*pixbuf++ = 0xff;
					break;
				case 16:
					shortPixel = * ( unsigned short * ) pixbuf;
					pixbuf += 2;
					*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
					*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
					*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
					*pixbuf++ = 0xff;
					break;
					
				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alpha = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alpha;
					break;
				default:
					Error( "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
					break;
			}
		}
		// rows are always 32 bit aligned
		rowStart += ( ( buf_p - rowStart ) + 3 ) &~3;
	}
	
	free( buffer );
}


//=====================================================================================

typedef struct TargaHeader_s {
	unsigned char 	id_length;
	unsigned char	colormap_type;
	unsigned char	image_type;
	unsigned short	colormap_index;
	unsigned short	colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin;
	unsigned short	y_origin;
	unsigned short	width, height;
	unsigned char	pixel_size;
	unsigned char	attributes;
} TargaHeaeder_t;

static const int TGA_HEADER_SIZE = 18;

/*
 ========================
 WriteTGA
 
 Write a TGA to a buffer.
 ========================
 */
void WriteTGA( byte **bufferOut, size_t *bufferSizeOut, const byte *data, int width, int height, 
				int sourceDepth, int flipVertical, int swapRGB ) {
	size_t	i;
	int     imgStart = TGA_HEADER_SIZE;
	
	assert( sourceDepth == 1 || sourceDepth == 3 || sourceDepth == 4 );
	
	size_t bufferSize = width * height * sourceDepth + TGA_HEADER_SIZE;
	*bufferSizeOut = bufferSize;
	
	byte *buffer = (byte*)malloc( bufferSize );
	*bufferOut = buffer;
	
	memset( buffer, 0, TGA_HEADER_SIZE );
	
	static const int TGA_IMAGETYPE_GREYSCALE = 3;
	static const int TGA_IMAGETYPE_RGB = 2;
	
	buffer[ 2 ]  = sourceDepth == 1 ? TGA_IMAGETYPE_GREYSCALE : TGA_IMAGETYPE_RGB;
	buffer[ 12 ] = width & 255;
	buffer[ 13 ] = width >> 8;
	buffer[ 14 ] = height & 255;
	buffer[ 15 ] = height >> 8;
	buffer[ 16 ] = sourceDepth * 8;	// pixel size
	if ( !flipVertical ) {
		buffer[ 17 ] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}
	
	if ( sourceDepth == 4 ) {
		if ( swapRGB ) {
			// swap rgb to bgr
			for ( i = imgStart ; i < bufferSize ; i += sourceDepth ) {
				buffer[ i ]		= data[ i - imgStart + 2 ];		// blue
				buffer[ i + 1 ] = data[ i - imgStart + 1 ];		// green
				buffer[ i + 2 ] = data[ i - imgStart ];			// red
				buffer[ i + 3 ] = data[ i - imgStart + 3 ];		// alpha
			}
		} else {
			memcpy( buffer + imgStart, data, bufferSize - TGA_HEADER_SIZE );
		}
	} else if ( sourceDepth == 3 ) {
		if ( swapRGB ) {
			for ( i = imgStart ; i < bufferSize ; i += sourceDepth ) {
				buffer[ i ]		= data[ i - imgStart + 2 ];		// blue
				buffer[ i + 1 ] = data[ i - imgStart + 1 ];		// green
				buffer[ i + 2 ] = data[ i - imgStart + 0 ];		// red
			}
		} else {
			for ( i = imgStart ; i < bufferSize ; i += sourceDepth ) {
				buffer[ i ]		= data[ i - imgStart ];			// blue
				buffer[ i + 1 ] = data[ i - imgStart + 1 ];		// green
				buffer[ i + 2 ] = data[ i - imgStart + 2 ];		// red
			}
		}
	} else if ( sourceDepth == 1 ) {
		memcpy( buffer + imgStart, data, bufferSize - TGA_HEADER_SIZE );
	}
}

void WriteTGAFile( const char *filename, const byte *pic, int w, int h ) {
	byte *buf;
	size_t	bufLen;
	WriteTGA( &buf, &bufLen, pic, w, h, 4, 0, 0 );
	FILE * f = fopen( filename, "wb" );
	assert(	f );
	fwrite( buf, bufLen, 1, f );
	fclose( f );
	free( buf );
}

/*
 ========================
 LoadTGAFromBuffer
 
 Load a TGA from a buffer containing a TGA file.
 ========================
 */
int LoadTGAFromBuffer( const char *name, const unsigned char *buffer, const int bufferSize, 
					  unsigned char **pic, int *width, int *height ) {
	int			columns, rows, numPixels;
	size_t		numBytes;
	unsigned char			*pixbuf;
	int			row, column;
	const unsigned char	*buf_p;
	struct TargaHeader_s	targa_header;
	unsigned char		*targa_rgba;
	
	*pic = NULL;
	
	buf_p = buffer;
	
	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	targa_header.colormap_index = *(short *)buf_p;
	buf_p += 2;
	targa_header.colormap_length = *(short *)buf_p;
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = *(short *)buf_p;
	buf_p += 2;
	targa_header.y_origin = *(short *)buf_p;
	buf_p += 2;
	targa_header.width = *(short *)buf_p;
	buf_p += 2;
	targa_header.height = *(short *)buf_p;
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;
	
	if ( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 ) {
		printf( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported", name );
		return 0;
	}
	
	if ( targa_header.colormap_type != 0 ) {
		printf( "LoadTGA( %s ): colormaps not supported", name );
		return 0;
	}
	
	if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 ) {
		printf( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)", name );
		return 0;
	}
	
	if ( targa_header.image_type == 2 || targa_header.image_type == 3 ) {
		numBytes = targa_header.width * targa_header.height * ( targa_header.pixel_size >> 3 );
		if ( numBytes > bufferSize - TGA_HEADER_SIZE - targa_header.id_length ) {
			printf( "LoadTGA( %s ): incomplete file", name );
			return 0;
		}
	}
	
	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;
	
	if ( width ) {
		*width = columns;
	}
	if ( height ) {
		*height = rows;
	}
	
	targa_rgba = (unsigned char *)malloc( numPixels*4 );
	*pic = targa_rgba;
	
	if ( targa_header.id_length != 0 ) {
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}
	
	if ( targa_header.image_type == 2 || targa_header.image_type == 3 ) { 
		unsigned char red,green,blue,alphabyte;
		switch( targa_header.pixel_size ) {
			case 8:
				// Uncompressed gray scale image
				for( row = rows - 1; row >= 0; row-- ) {
					pixbuf = targa_rgba + row*columns*4;
					for( column = 0; column < columns; column++ ) {
						blue = *buf_p++;
						green = blue;
						red = blue;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
					}
				}
				break;
			case 24:
				// Uncompressed RGB image
				for( row = rows - 1; row >= 0; row-- ) {
					pixbuf = targa_rgba + row*columns*4;
					for( column = 0; column < columns; column++ ) {
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
					}
				}
				break;
			case 32:
				// Uncompressed RGBA image
				for( row = rows - 1; row >= 0; row-- ) {
					pixbuf = targa_rgba + row*columns*4;
					for( column = 0; column < columns; column++ ) {
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
					}
				}
				break;
			default:
				printf( "LoadTGA( %s ): illegal pixel_size '%d'", name, targa_header.pixel_size );
				free( *pic );
				*pic = NULL;
				return 0;
		}
	}
	else if ( targa_header.image_type == 10 ) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		
		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;
		
		for( row = rows - 1; row >= 0; row-- ) {
			pixbuf = targa_rgba + row*columns*4;
			for( column = 0; column < columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if ( packetHeader & 0x80 ) {        // run-length packet
					switch( targa_header.pixel_size ) {
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							break;
						default:
							printf( "LoadTGA( %s ): illegal pixel_size '%d'", name, targa_header.pixel_size );
							free( *pic );
							*pic = NULL;
							return 0;
					}
					
					for( j = 0; j < packetSize; j++ ) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if ( column == columns ) { // run spans across rows
							column = 0;
							if ( row > 0) {
								row--;
							}
							else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				} else {                            // non run-length packet
					for( j = 0; j < packetSize; j++ ) {
						switch( targa_header.pixel_size ) {
							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default:
								printf( "LoadTGA( %s ): illegal pixel_size '%d'", name, targa_header.pixel_size );
								free( *pic );
								*pic = NULL;
								return 0;
						}
						column++;
						if ( column == columns ) { // pixel packet run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							}
							else {
								goto breakOut;
							}
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
		breakOut: ;
		}
	}
	
	if ( (targa_header.attributes & (1<<5)) ) {			// image flp bit
		byte *temp = malloc( *width * *height * 4 );
		memcpy( temp, *pic, *width * *height * 4 );
		
		for ( int y = 0 ; y < *height ; y++ ) {
			memcpy( *pic + y * *width * 4, temp + (*height-1-y) * *width * 4, *width * 4 );
		}
		free( temp );
	}
	
	return 1;
}

/*
 ========================
 LoadTGA
 
 Load TGA directly from a file.
 ========================
 */
int LoadTGA( const char *name, unsigned char **pic, int *width, int *height ) {
	int		len;
	unsigned char		*buf;
	int		ret;
	
	FILE	*f = fopen( name, "rb" );
	if ( !f ) {
		return 0;
	}
	len = FileLength( f );
	buf = malloc( len );
	fread( buf, 1, len, f );
	fclose( f );
	
	ret = LoadTGAFromBuffer( name, buf, len, pic, width, height );
	
	free( buf );
	
	return ret;
}

void OutlineImage( unsigned char *rgba, int width, int height ) {
	unsigned char *data_p;
	unsigned char *copy_p;
	unsigned char	*copy = (unsigned char *)alloca( width * height * 4 );
	int		x, y;
	
	memcpy( copy, rgba, width * height * 4 );
	data_p = rgba;
	copy_p = copy;
	
	for ( y = 0 ; y < height ; y++ ) {
		for ( x = 0 ; x < width ; x++, data_p+=4, copy_p+=4 ) {
			if ( data_p[3] != 0 ) {
				continue;
			}
			if ( x < width-1 && copy_p[7] != 0 ) {
				*(int *)data_p = ((int *)copy_p)[1];
			} else if ( x > 0 && copy_p[-1] != 0 ) {
				*(int *)data_p = ((int *)copy_p)[-1];
			} else if ( y < height-1 && copy_p[width*4+3] != 0 ) {
				*(int *)data_p = ((int *)copy_p)[width];
			} else if ( y > 0 && copy_p[-width*4+3] != 0 ) {
				*(int *)data_p = ((int *)copy_p)[-width];
			}
			data_p[3] = 1;
		}
	}
}

int RowClear( unsigned char *rgba, int w, int h, int y ) {
	int	x;
	for ( x = 0 ; x < w ; x++ ) {
		if ( rgba[(y*w+x)*4+3] != 0 ) {
			return 0;
		}
	}
	return 1;
}



int NextPowerOfTwo( int n ) {
	int	p = 1;
	
	while ( p < n ) {
		p <<= 1;
	}
	return p;
}

/*
 ========================
 AddTGA
 
 ========================
 */
void AddTGA( const char *localName, const byte *data, int dataLen ) {
	assert( buildHeader.textures.count < MAX_IMAGE_TABLE );
	pkTextureData_t *image = &buildTextureTable[buildHeader.textures.count++];
	strcpy( image->name.name, localName );
	image->picDataOfs = ftell( pakFile );
	
	// load it
	unsigned char *pic;
	int			width, height;
	
	if ( !LoadTGAFromBuffer( localName, data, dataLen, &pic, &width, &height ) ) {
		Error( "failed.\n" );
	}
	
	// scan for alpha
	int hasAlpha = 0;
	for ( int i = 0 ; i < width*height ; i++ ) {
		if ( pic[i*4+3] != 255 ) {
			hasAlpha = 1;
			break;
		}
	}

	// default image format
	image->format = TF_5551;
		
	// scan the parmLines for this filename
	for ( int i = 0 ; i < numParmLines ; i++ ) {
		if ( !strcasecmp( parmLines[i].argv[0], localName ) ) {
			for ( int j = 1 ; j < parmLines[i].argc ; j++ ) {
				if ( !strcmp( parmLines[i].argv[j], "5551" ) ) {
					image->format = TF_5551;
				} else if ( !strcmp( parmLines[i].argv[j], "4444" ) ) {
					image->format = TF_4444;
				} else if ( !strcmp( parmLines[i].argv[j], "565" ) ) {
					image->format = TF_565;
				} else if ( !strcmp( parmLines[i].argv[j], "8888" ) ) {
					image->format = TF_8888;
				} else if ( !strcmp( parmLines[i].argv[j], "LA" ) ) {
					image->format = TF_LA;
				} else if ( !strcmp( parmLines[i].argv[j], "PVR4" ) ) {
					if ( hasAlpha ) {
						image->format = TF_PVR4;
					} else {
						image->format = TF_PVR4A;
					}
				} else if ( !strcmp( parmLines[i].argv[j], "PVR2" ) ) {
					if ( hasAlpha ) {
						image->format = TF_PVR2;
					} else {
						image->format = TF_PVR2A;
					}
				} else {
					printf( "bad parm '%s'\n", parmLines[i].argv[j] );
				}
			}
			break;
		}
	}
	
	
	
	// set this true if we need to write a new tga out for compression
	// because we modified it in some way from the original (make power of 2, sprite outline, etc)
	int imageModified = 0;
	
	// make sure it is a power of two
	int	potW = NextPowerOfTwo( width );
	int potH = NextPowerOfTwo( height );

	// the texturetool compressor only supports square textures as of iphone OS 2.2.1
	// Not sure if that is a hardware limit or just software.  This throws away
	// some of the space savings, but it is still a speed savings to use.
	if ( image->format == TF_PVR4 || image->format == TF_PVR2 ) {
		if ( potW > potH ) {
			potH = potW;
		}
		if ( potH > potW ) {
			potW = potH;
		}
	}
	
	if ( potW > width || potH > height ) {
		printf( "Insetting %i x %i image in %i x %i block\n", width, height, potW, potH );
		unsigned char *newPic = (unsigned char *)malloc( potW * potH * 4 );
		// replicating the last row or column might be better
		if ( hasAlpha ) {
			memset( newPic, 0, potW * potH * 4 );
		} else {
			memset( newPic, 255, potW * potH * 4 );
		}
		for ( int y = 0 ; y < height ; y++ ) {
			memcpy( newPic + y * potW * 4, pic + y * width * 4, width * 4 );
		}
		free( pic );
		pic = newPic;
		imageModified = 1;
	}
	
	image->srcWidth = width;
	image->srcHeight = height;
	image->uploadWidth = potW;
	image->uploadHeight = potH;

	image->wrapS = GL_REPEAT;
	image->wrapT = GL_REPEAT;
	image->minFilter = GL_LINEAR_MIPMAP_NEAREST;
	image->magFilter = GL_LINEAR;
	image->aniso = 1;
	image->numLevels = 0;
	image->maxS = (float)image->srcWidth / image->uploadWidth;
	image->maxT = (float)image->srcHeight / image->uploadHeight;
	
	int	w = image->uploadWidth;
	int	h = image->uploadHeight;
	
	// determine the number of mip levels.  We can't just count as
	// we create them, because the PVRTC texturetool creates them
	// all in one run
	int max = w > h ? w : h;
	while ( max >= 1 ) {
		image->numLevels++;
		max >>= 1;
	}
	
	// checkerboard debug tool for testing texel centers
	int	checker = 0;
	if ( checker ) {
		for ( int y = 0 ; y < height ; y++ ) {
			for ( int x = 0 ; x < width ; x++ ) {
				if ( (x^y)&1 ) {
					*((int *)pic+y*potW+x) = -1;
				} else {
					*((int *)pic+y*potW+x) = 0;
				}
			}
		}
		imageModified = 1;
	}

	// sprite image outlining to avoid bilinear filter halos
	int sprite = 0;
	if ( sprite ) {
		for ( int i = 0 ; i < 8 ; i++ ) {
			OutlineImage( pic, width, height );
		}
		for ( int i = 0 ; i < width*height ; i++ ) {
			if ( pic[i*4+3] == 1 ) {
				pic[i*4+3] = 0;
			}
		}
		imageModified = 1;
	}

	//-----------------------------------------
	// scan for bounding box of opaque texels
	//-----------------------------------------
	if ( !hasAlpha ) {
		image->numBounds = 0;
	} else {
		int	x, y;
		
		// find the bounding boxes for more efficient drawing
		image->numBounds = 1;
		for ( y = 0 ; y < h ; y++ ) {
			if ( !RowClear( pic, w, h, y ) ) {
				// this row is needed
				image->bounds[0][0][1] = y;
				break;
			}
		}
		for ( y = h-1 ; y >= 0 ; y-- ) {
			if ( !RowClear( pic, w, h, y ) ) {
				// this row is needed
				image->bounds[0][1][1] = y;
				break;
			}
		}
		
		// if the middle row is clear, make two boxes
		// We could make a better test, but this catches the ones we care about...
		if ( image->bounds[0][0][1] < h/2 && image->bounds[0][1][1] > h / 2 
			&& RowClear( pic, w, h, h/2 ) ) {
			image->numBounds = 2;
			image->bounds[1][1][1] = image->bounds[0][1][1];
			
			for ( y = h/2-1 ; y >= 0 ; y-- ) {
				if ( !RowClear( pic, w, h, y ) ) {
					image->bounds[0][1][1] = y;
					break;
				}
			}
			for ( y = h/2+1 ; y < h ; y++ ) {
				if ( !RowClear( pic, w, h, y ) ) {
					image->bounds[1][0][1] = y;
					break;
				}
			}
		}
		
		for ( int b = 0 ; b < image->numBounds ; b++ ) {
			for ( x = 0 ; x < w ; x++ ) {
				for ( y = image->bounds[b][0][1] ; y <= image->bounds[b][1][1] ; y++ ) {
					if ( pic[(y*w+x)*4+3] != 0 ) {
						// this column is needed
						image->bounds[b][0][0] = x;
						break;
					}
				}
				if ( y <= image->bounds[b][1][1] ) {
					break;
				}
			}
			for ( x = w-1 ; x >= 0 ; x-- ) {
				for ( y = image->bounds[b][0][1] ; y <= image->bounds[b][1][1] ; y++ ) {
					if ( pic[(y*w+x)*4+3] != 0 ) {
						// this column is needed
						image->bounds[b][1][0] = x;
						break;
					}
				}
				if ( y <= image->bounds[b][1][1] ) {
					break;
				}
			}
		}
	}
	
	//-----------------------------------------
	// run texturetool to PVR compress and generate all mip levels
	// Arguably, we should do the sprite outlining on each mip level
	// independently, and PVR compress each layer seperately.
	//-----------------------------------------
	if ( image->format == TF_PVR4 || image->format == TF_PVR2 
		|| image->format == TF_PVR4A || image->format == TF_PVR2A ) {
		char	tempTGAname[L_tmpnam];
		
		// write the modified image data out if necessary
		if ( imageModified ) {
			tmpnam( tempTGAname );

			WriteTGAFile( tempTGAname, pic, w, h );
		} else {
			sprintf( tempTGAname, "%s/%s", assetDirectory, localName );
		}
		
		// run the external compression tool
		// FIXME: use an explicit name and timestamp check
		char	tempPVRname[L_tmpnam];
		tmpnam( tempPVRname );
		char	cmd[1024];
		sprintf( cmd, "/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/texturetool -m -e PVRTC %s -f Raw -o %s %s",
				( image->format == TF_PVR2 || image->format == TF_PVR2A ) ? "--bits-per-pixel-2" : "--bits-per-pixel-4", 
				tempPVRname, tempTGAname );
		printf( "%s\n", cmd );
		system( cmd );
		
		FILE *f = fopen( tempPVRname, "rb" );
		if ( !f ) {
			Error( "Can't open '%s'\n", tempPVRname );
		}
		int len = FileLength( f );
		unsigned char *raw = alloca( len );
		fread( raw, 1, len, f );
		fclose( f );
		
		// write to the pak file
		fwrite( raw, 1, len, pakFile );

		if ( imageModified ) {
			remove( tempTGAname );
		}
		remove( tempPVRname );
		return;
	}

	//-----------------------------------------
	// create mip maps and write out simple image formats
	//-----------------------------------------		
	while ( 1 ) {
		byte *rgba_p = pic;

		// convert to target format
		switch ( image->format ) {
			case TF_8888:
			{
				int * processed = alloca( w * h * 4 );
				int * s_p = processed;
				
				for ( int i = 0 ; i < w*h ; i++, rgba_p+=4 ) {
					int	r = rgba_p[0];
					int	g = rgba_p[1];
					int	b = rgba_p[2];
					int	a = rgba_p[3];
					
					*s_p++ = (b<<24) | (g<<16) | (r<<8) | a;
				}
				// write it out
				fwrite( processed, w * h, 4, pakFile );
				break;
			}
			case TF_LA:
			{
				byte * processed = alloca( w * h * 2 );
				byte * s_p = processed;
				
				for ( int i = 0 ; i < w*h ; i++, rgba_p+=4 ) {
					int	l = rgba_p[0];
					int	a = rgba_p[1];	// this should probably be [3], but Cass's font renderer saved it out as LA01
					
					*s_p++ = l;
					*s_p++ = a;
				}
				// write it out
				fwrite( processed, w * h, 2, pakFile );
				break;
			}
			case TF_5551:
			{
				short * processed = alloca( w * h * 2 );
				short * s_p = processed;
				
				for ( int i = 0 ; i < w*h ; i++, rgba_p+=4 ) {
					int	r = rgba_p[0];
					int	g = rgba_p[1];
					int	b = rgba_p[2];
					int	a = rgba_p[3];
					
					*s_p++ = ((r>>3)<<11) | ((g>>3)<<6) | ((b>>3)<<1) | (a>>7);
				}
				// write it out
				fwrite( processed, w * h, 2, pakFile );
				break;
			}
			case TF_565:
			{
				short * processed = alloca( w * h * 2 );
				short * s_p = processed;
				
				for ( int i = 0 ; i < w*h ; i++, rgba_p+=4 ) {
					int	r = rgba_p[0];
					int	g = rgba_p[1];
					int	b = rgba_p[2];
					
					*s_p++ = ((r>>3)<<11) | ((g>>2)<<5) | (b>>3);
				}
				// write it out
				fwrite( processed, w * h, 2, pakFile );
				break;
			}
			case TF_4444:
			{
				short * processed = alloca( w * h * 2 );
				short * s_p = processed;
				
				for ( int i = 0 ; i < w*h ; i++, rgba_p+=4 ) {
					int	r = rgba_p[0];
					int	g = rgba_p[1];
					int	b = rgba_p[2];
					int	a = rgba_p[3];
					
					*s_p++ = ((r>>4)<<12) | ((g>>4)<<8) | ((b>>4)<<4) | (a>>4);
				}
				// write it out
				fwrite( processed, w * h, 2, pakFile );
				break;
			}
			default:
				Error( "unimplemented format: %i\n", image->format );
		}
				
		if ( w == 1 && h == 1 ) {
			break;
		}
		// mip map
		w >>= 1;
		if ( w == 0 ) {
			w = 1;
		}
		h >>= 1;
		if ( h == 0 ) {
			h = 1;
		}
		byte *tempMip = alloca( w * h * 4 );
		// FIXME: doesn't handle 2x1 and 1x2 cases properly...
		for ( int y = 0 ; y < h ; y++ ) {
			for ( int x = 0 ; x < w ; x++ ) {
				for ( int c = 0 ; c < 4 ; c++ ) {
					tempMip[(y*w+x)*4+c] = (
											pic[((y*2+0)*w*2+(x*2+0))*4+c] +
											pic[((y*2+0)*w*2+(x*2+1))*4+c] +
											pic[((y*2+1)*w*2+(x*2+0))*4+c] +
											pic[((y*2+1)*w*2+(x*2+1))*4+c] ) >> 2;
				}
			}
		}
		pic = tempMip;
	}
	
}

/*
 ========================
 AddRAW
 
 ========================
 */
void AddRAW( const char *localName, const byte *data, int dataLen ) {
	assert( buildHeader.raws.count < MAX_RAW_TABLE );
	pkRawData_t *raw = &buildRawTable[buildHeader.raws.count++];
	strcpy( raw->name.name, localName );
	raw->rawDataOfs = ftell( pakFile );
	raw->rawDataLen = dataLen;
	
	fwrite( data, 1, dataLen, pakFile );
	
	// always add a 0 after each raw file so text files can be assumed to be
	// c-string terminated
	byte	zero = 0;
	fwrite( &zero, 1, 1, pakFile );
}

/*
 ========================
 AddDirectoryToPak_r
 
 ========================
 */
void AddDirectoryToPak_r( const char *localDirName ) {
	char	fullDirName[MAXPATHLEN];
	
	if ( localDirName[0] == '/' ) {
		localDirName++;
	}
	sprintf( fullDirName, "%s/%s", assetDirectory, localDirName );
	printf( "entering %s\n", fullDirName );
	DIR *dir = opendir( fullDirName );
	assert( dir );
	
	while( 1 ) {
		// make sure the file pointer is 16 byte aligned, since
		// we will be referencing it with mmap.  Alignment greater than
		// 4 might be wasted on iPhone, but it won't be all that much space.
		int	ofs = ftell( pakFile );
		if ( ofs & 15 ) {
			byte	pad[16];
			memset( pad, 0, sizeof( pad ) );
			fwrite( pad, 16 - ( ofs & 15 ), 1, pakFile );
		}
		
		// get the next file in the directory
		struct dirent *file = readdir( dir );
		if ( !file ) {
			return;
		}
		
		char	localFileName[MAXPATHLEN];
		if ( localDirName[0] ) {
			sprintf( localFileName, "%s/%s", localDirName, file->d_name );
		} else {
			sprintf( localFileName, "%s", file->d_name );
		}
		
		if ( file->d_name[0] == '.' ) {
			// ignore . and .. and hidden files
			continue;
		}
		if ( file->d_type == DT_DIR ) {
			// recurse into another directory
			AddDirectoryToPak_r( localFileName );
			continue;
		}
		
		// make sure name length fits
		assert( strlen( localFileName ) < MAX_PK_NAME - 1 );
		
		// load the file
		char	fullFileName[MAXPATHLEN];
		sprintf( fullFileName, "%s/%s", assetDirectory, localFileName );
		FILE *f = fopen( fullFileName, "rb" );
		if ( !f ) {
			Error( "Can't open '%s'\n", localFileName );
		}
		int len = FileLength( f );
		unsigned char *raw = malloc( len );
		fread( raw, 1, len, f );
		fclose( f );
		printf( "%8i %s\n", len, localFileName );
		if ( strstr( localFileName, ".tga" ) ) {
			AddTGA( localFileName, raw, len );
		} else if ( strstr( localFileName, ".wav" ) ) {
			AddWAV( localFileName, raw, len );
		} else {
			AddRAW( localFileName, raw, len );
		}
		free( raw );
	}
}

//======================================================================================

#define	ATLAS_SIZE	1024
#define ATLAS_EMPTY_ALPHA 128

byte	atlas[ATLAS_SIZE*ATLAS_SIZE*4];
int		atlasNum = 0;

int FindSpotInAtlas( int w, int h, int *spotX, int *spotY ) {
	int		x = 0;
	int		y = 0;
	int		maxX = ATLAS_SIZE - w;
	int		maxY = ATLAS_SIZE - h;
	
	while( 1 ) {
	retry:		
		for ( int yy = 0 ; yy < h ; yy++ ) {
			for ( int xx = 0 ; xx < w ; xx++ ) {
				if ( atlas[((y+yy)*ATLAS_SIZE+x+xx)*4+3] != ATLAS_EMPTY_ALPHA ) {
					// can't use this spot, skip ahead past this solid mark
					x = x + xx + 1;
					if ( x > maxX ) {
						x = 0;
						y++;
						if ( y > maxY ) {
							return 0;
						}
					}
					goto retry;
				}
			}
		}
		*spotX = x;
		*spotY = y;
		return 1;
	}
	return 0;
}

void EmptyAtlas() {
	// fill with alpha 128 to signify empty
	memset( atlas, 0, sizeof( atlas ) );
	for ( int i = 0 ; i < ATLAS_SIZE * ATLAS_SIZE ; i++ ) {
		atlas[i*4+3] = ATLAS_EMPTY_ALPHA;
	}
}

void ClearBlock( int x, int y, int w, int h ) {
	// fill with black / alpha 0
	for ( int yy = 0 ; yy < h ; yy++ ) {
		memset( atlas + ((y+yy)*ATLAS_SIZE+x)*4, 0, w*4 );
	}
}

void FinishAtlas() {
	char	filename[1024];
	
	sprintf( filename, "%s/atlas%i.tga", assetDirectory, atlasNum );
	printf( "Writing %s.\n", filename );
	WriteTGAFile( filename, atlas, ATLAS_SIZE, ATLAS_SIZE );
	// this atlas is complete, write it out
	atlasNum++;
	// clear it and retry the allocation
	EmptyAtlas();
}


/*
 ========================
 AtlasDirectory
 
 ========================
 */
void AtlasDirectory( const char *fullDirName, const char *prefix ) {
	printf( "atlasing %s* from %s\n", prefix, fullDirName );
	DIR *dir = opendir( fullDirName );
	assert( dir );
	
	int		totalSourceTexels = 0;
	int		totalSourceImages = 0;
	int		totalBorderedSourceTexels = 0;
	int		totalPotTexels = 0;
	
	EmptyAtlas();
	
	while( 1 ) {
		// get the next file in the directory
		struct dirent *file = readdir( dir );
		if ( !file ) {
			break;
		}
		if ( file->d_name[0] == '.' ) {
			// ignore . and .. and hidden files
			continue;
		}
#if 0		
		if ( file->d_type == DT_DIR ) {
			// recurse into another directory
			AddDirectoryToPak_r( localFileName );
			continue;
		}
#endif		
		if ( !strstr( file->d_name, ".BMP" ) && !strstr( file->d_name, ".bmp" ) ) {
			continue;
		}
		
		// only grab the specified images
		if ( strncmp( file->d_name, prefix, strlen( prefix ) ) ) {
			continue;
		}
			
		// load the image
		char	fullFileName[MAXPATHLEN];
		sprintf( fullFileName, "%s/%s", fullDirName, file->d_name );
		
		byte *pic;
		int	width, height;
		LoadBMP( fullFileName, &pic, &width, &height );
		
		// add a four pixel border around each sprite for mip map outlines
		static const int OUTLINE_WIDTH = 4;
		int widthInAtlas = width + 2*OUTLINE_WIDTH;
		int heightInAtlas = height + 2*OUTLINE_WIDTH;
		
		int	ax, ay;
		
		if ( !FindSpotInAtlas( widthInAtlas, heightInAtlas, &ax, &ay ) ) {
			FinishAtlas();
			if ( !FindSpotInAtlas( widthInAtlas, heightInAtlas, &ax, &ay ) ) {
				Error( "Couldn't allocate %s: %i,%i in empty atlas", fullFileName, width, height );
			}
		}
		
		printf( "%4i, %4i at %4i,%4i: %s\n", width, height, ax, ay, fullFileName );
		totalSourceTexels += width * height;
		totalSourceImages++;
		totalBorderedSourceTexels += widthInAtlas * heightInAtlas;
		totalPotTexels += NextPowerOfTwo( width ) * NextPowerOfTwo( height );
		
		// clear the extended border area to fully transparent
		ClearBlock( ax, ay, widthInAtlas, heightInAtlas );
		
		// copy the actual image into the inset area past the added borders
		// for Doom graphics, the color key alpha value is always the top left corner texel
		ax += OUTLINE_WIDTH;
		ay += OUTLINE_WIDTH;
		for ( int y = 0 ; y < height ; y++ ) {
			for ( int x = 0 ; x < width ; x++ ) {
				int	p = ((int *)pic)[y*width+x];
				if ( p == DOOM_ALPHA_TEXEL ) {
					((int *)atlas)[ (ay+y)*ATLAS_SIZE+ax+x ] = 0;
				} else {
					((int *)atlas)[ (ay+y)*ATLAS_SIZE+ax+x ] = p;
				}
			}
		}
	}
	
	// process and write out the partially filled atlas
	FinishAtlas();
	
	printf ("%i soource images\n", totalSourceImages );
	printf ("%i atlas images\n", atlasNum );
	printf ("%6.1fk source texels\n", totalSourceTexels*0.001f );
	printf ("%6.1fk bordered source texels\n", totalBorderedSourceTexels*0.001f );
	printf ("%6.1fk atlas texels\n", atlasNum*ATLAS_SIZE*ATLAS_SIZE*0.001f );
	printf ("%6.1fk power of two inset texels\n", totalPotTexels*0.001f );
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
 ========================
 WriteType
  
 ========================
 */
void WriteType( FILE *pakFile, pkType_t *type, int structSize, pkName_t *table ) {
	type->structSize = structSize;
	type->tableOfs = ftell( pakFile );

	// build hash chains for everything
	for ( int i = 0 ; i < PK_HASH_CHAINS ; i++ ) {
		type->hashChains[i] = -1;
	}
	for ( int i = 0 ; i < type->count ; i++ ) {
		pkName_t *name = (pkName_t *)((unsigned char *)table + i * structSize );
		char	original[MAX_PK_NAME];
		strcpy( original, name->name );
		// make the name canonical and get the hash
		name->nameHash = PK_HashName( original, name->name );
		
		// add it to the hash chain
		int chain = name->nameHash & (PK_HASH_CHAINS-1);
		name->nextOnHashChain = type->hashChains[chain];
		type->hashChains[chain] = i;
	}
	
	fwrite( table, type->count, type->structSize, pakFile );
}

/*
 ========================
 main
 
 ========================
 */
int main (int argc, const char * argv[]) {
	int	arg;
	
	for ( arg = 1 ; arg < argc ; arg++ ) {
		if ( argv[arg][0] != '-' ) {
			break;
		}
		if ( !strcmp( argv[arg], "-i" ) ) {
			assetDirectory = argv[arg+1];
			arg++;
			continue;
		}
		if ( !strcmp( argv[arg], "-o" ) ) {
			outputFile = argv[arg+1];
			arg++;
			continue;
		}
		if ( !strcmp( argv[arg], "-p" ) ) {
			parmFile = argv[arg+1];
			arg++;
			continue;
		}
		if ( !strcmp( argv[arg], "-?" ) ) {
			Error( "doomtool [-i inputDirectory] [-o outputFile] [-p parmfile]\n" );
		}
		Error( "unknown option '%s'\n", argv[arg] );
	}

	//-----------------------------
	// parse the parm file
	//-----------------------------
	FILE *f = fopen( parmFile, "rb" );
	numParmLines = 0;
	if ( f ) {
		char	line[1024];
		while( fgets( line, sizeof( line ), f ) ) {
			// remove trailing newline
			if ( line[strlen(line)-1] == '\n' ) {
				line[strlen(line)-1] = 0;
			}
			
			parmLine_t *pl = &parmLines[numParmLines];
			// tokenize
			char *inputString = line;
			char *ap;
			while( ap = strsep( &inputString, " \t" ) ) {
				if ( *ap == '\0' ) {
					continue;
				}
				pl->argv[pl->argc] = strdup( ap );
				if ( ++pl->argc == MAX_ARGV ) {
					break;
				}
			}
			if ( pl->argc > 0 ) {
				numParmLines++;
			}
		}
		fclose( f );
	}
	
//	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "" );
#if 0
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "BOS2" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "BOSS" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "BSPI" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "CPOS" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "CYBR" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "FAT" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "HEAD" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "PAIN" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "PLAY" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "POSS" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SARG" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SKEL" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SKUL" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SPID" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SPOS" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "SSWV" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "TROO" );
	AtlasDirectory( "/Users/johnc/DOOM2/SPRITES", "VILE" );
#endif	
	//-----------------------------
	// start writing the outputFile
	//-----------------------------
	
	pakFile = fopen( outputFile, "wb" );
	assert( pakFile );
	
	// leave space for the header, which will be written at the end
	fwrite( &buildHeader, 1, sizeof( buildHeader ), pakFile );
	
	// recursively process everything under the asset directory
	AddDirectoryToPak_r( "" );
	
	// write out the tables
	WriteType( pakFile, &buildHeader.textures, sizeof( pkTextureData_t ), &buildTextureTable[0].name );
	WriteType( pakFile, &buildHeader.wavs, sizeof( pkWavData_t ), &buildWavTable[0].name );
	WriteType( pakFile, &buildHeader.raws, sizeof( pkRawData_t ), &buildRawTable[0].name );
	
	buildHeader.version = PKFILE_VERSION;
	
	printf( "%s : %i bytes\n", outputFile, ftell( pakFile ) );
	
	// go back and write the header
	fseek( pakFile, 0, SEEK_SET );
	fwrite( &buildHeader, 1, sizeof( buildHeader ), pakFile );
	
	fclose( pakFile );
	
    return 0;
}
