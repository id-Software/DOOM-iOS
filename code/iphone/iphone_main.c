/*

	Copyright (C) 2004-2005 Michael Liebscher <johnnycanuck@users.sourceforge.net>
	Copyright (C) 1997-2001 Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


/*

 doom
 ----
 modifications to config.h
 don't add the d_ipxgate.c file
 don't add d_server.c file
 don't add mmus2mid.[ch] files
 don't add w_memcache.c, use w_mmap.c instead
 added new SDL_opengl.h, changed code files from <SDL_opengl.h> to "SDL_opengl.h"
 commented out #include <SDL.h>

 Commented out the static on D_DoomMainSetup();
 Add define HAVE_CONFIG_H to the target settings
 Add define _DEBUG 
 
 #if around uses of GL_TEXTURE_RESIDENT in gl_texture.c
 // JDC: not in GLES, not needed since it is the default condition  glDisable(GL_POLYGON_SMOOTH);

 // JDC #define USE_VERTEX_ARRAYS in gl_main.c

 add the iphoneRotateForLandscape calls
 
static JDC removed short    consistancy[MAXPLAYERS][BACKUPTICS];

 // JDC  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 // JDC  glDisableClientState(GL_VERTEX_ARRAY);
 #ifdef IPHONE			// JDC
 #define MAX_SCREENWIDTH  480
 #define MAX_SCREENHEIGHT 320
 #else
 
 
 code notes
 ----------
 all the fixed point mess
 goofy polar clipping
 path traverse should have used a BSP tree
 
 rename vldoor_e close and open enums to not conflict with unistd
 
 opengl_error_break
 
 
 WiFi internet play
 ---------
 voip instead of key chat

 extra asset work
 ----------------------
 perfect weapon scales
 demo of each level?
 additive art separation -- attacks, buttons, etc
 
 tuned option
 ------------
 last kill / last secret / last item messages
 more achievements in general
 better item pickup sounds
 scale blood spray with distance
 easier item pickup
 crosshair
 better movement clipping to walls
 better key pickup sound
 textured automap
 better low health warning
 better feedback on where bullets hit
 better bob cycle when going down stairs
 map item should also show all monsters and items to make it cool
 barrels explode easier
 wave FOV when berzerk
 pistol shots are useless in deathmatch
 show opponent / enemy health when attacking them?

 
 cpu optimizations
 -------------
 convert to 16 bit vertex data
 void gld_BindPatch(GLTexture *gltexture, int cm) is expensive
 fixedMul / fixedDiv in asm (negligable performance gain)
 atlas all of the non-character items (and bodies?)
 remove the pitch changes from sound?
 
 gpu optimizations
 ------------------
 use fog instead of full screen blend for damage/pickup flash (3 msec on IP3G)
 pvrtc walls and floors
  
 art needed
 ----------
 new map / menu button
 awards on medals
 icon
 thumbsticks
 settings gears
 
 
 must do
 -------
 pause music in menu
 timelimit carry over to new levels
 server going to demo causes client to crash
 
 fixed 10/13/09
 --------------
 recover from sound interruption
 caps for slider bar text
 reset defaults doesn't reset reverse-landscape
 pause music when going to the menu
 reset timelimit each level on deathmatch
 disable demos, new game, and web site during multiplayer
 
 fixed
 -----
 automap / menu button actions with multi-thread
 capitalized "game saved."
 Added punctuation to "Ned a blue keycard", etc
 added "defaults" button in options
 
 todo
 ----
 don't accept fire from an owned touch?
 text scaling in buttons isn't perfect
 better view angle transport
 audio bugs
 rotor control shouldn't be dimmed
 volume button hack?
 change background color for networking
 tapping weapon change to cycle
 is openal thread safe?  we issue touch clicks from asynctic
 disable multiplayer button if no ethernet addresses found
 touch latching issues
 add one tic latency to server?
 select new game while in netgame
 multiplayer arrow colors
 remove players with stale joins
 rocket explosion offset
 texture preload status bar stuff and blood / impacts
 stereo panning for headphones
 directly build 16 bit textures instead of translating from 32 bit
 texture wrap seam after end of e3m8
 sliders should be touch-latch controls
 respawn flash sounds sometimes not playing?
 don't allow starting on secret levels
 loaded savegame spot on different level didn't get view height on first frame
 use graphic?
 don't ever close doors with auto-use
 separators in map select
 rotor speed adjust
 check all powerup effects
 remove uses of prboom.wad?
 check patch outlining code
 flash all controls on initial level load
 touch sounds when cancelling demo playback
 play sound on respawn
 catch memory warning, purge textures
 use wad sound data
 stop sound cleanly
 entire gun doesn't get fullbright with muzzle flash
 require four touches in line for console
 restartable pwad interface
 somewhat normal based lighting on walls
 help menu
 visual tilt indicator
 tilt draw the turnstick when active
 console
 
 */

#include "doomiphone.h"

cvar_t  *freeLevelOfWeek;
cvar_t	*skill;
cvar_t	*episode;
cvar_t	*controlScheme;
cvar_t	*stickTurn;
cvar_t	*stickMove;
cvar_t	*stickDeadBand;
cvar_t	*rotorTurn;
cvar_t	*tiltTurn;
cvar_t	*tiltMove;
cvar_t	*tiltDeadBand;
cvar_t	*tiltAverages;
cvar_t	*miniNet;
cvar_t	*music;
cvar_t	*showTilt;
cvar_t	*showTime;
cvar_t	*showNet;
cvar_t	*showSound;
cvar_t	*cropSprites;
cvar_t	*mapScale;
cvar_t	*drawControls;
cvar_t	*autoUse;
cvar_t	*statusBar;
cvar_t	*touchClick;
cvar_t	*messages;
cvar_t	*timeLimit;
cvar_t	*fragLimit;
cvar_t	*mpDeathmatch;
cvar_t	*mpSkill;
cvar_t	*mpDataset;
cvar_t	*mpEpisode;
cvar_t	*mpMap;
cvar_t	*mpExpansion;
cvar_t	*noBlend;
cvar_t	*glfinish;
cvar_t	*mapSelectY;
cvar_t	*throttle;
cvar_t	*centerSticks;
cvar_t	*rampTurn;
cvar_t	*netBuffer;
cvar_t   *iwadSelection;
cvar_t   *pwadSelection;

char* doom_iwad;
char* doom_pwads = "";

#define VERSION_BCONFIG	( 0x89490000 + sizeof( huds ) + sizeof( playState ) )

void Sys_Error( const char *format, ... )
{ 
    va_list     argptr;
    char        string[ 1024 ];

    va_start( argptr, format );
    (void)vsnprintf( string, sizeof( string ), format, argptr );
    va_end( argptr );

	fprintf( stderr, "Error: %s\n", string );

	_exit( 1 );

} 

#define plyr (players+consoleplayer)     /* the console player */

void Give_f() {
	
	plyr->armorpoints = idfa_armor;      // Ty 03/09/98 - deh
	plyr->armortype = idfa_armor_class;  // Ty 03/09/98 - deh
	
	// You can't own weapons that aren't in the game // phares 02/27/98
	for (int i=0;i<NUMWEAPONS;i++)
		if (!(((i == wp_plasma || i == wp_bfg) && gamemode == shareware) ||
			  (i == wp_supershotgun && gamemode != commercial)))
			plyr->weaponowned[i] = true;
	
	for (int i=0;i<NUMAMMO;i++)
		if (i!=am_cell || gamemode!=shareware)
			plyr->ammo[i] = plyr->maxammo[i];
	
	plyr->message = s_STSTR_FAADDED;	

	for (int i=0;i<NUMCARDS;i++)
		if (!plyr->cards[i])     // only print message if at least one key added
		{                      // however, caller may overwrite message anyway
			plyr->cards[i] = true;
		}
}

void God_f() {
	plyr->cheats ^= CF_GODMODE;
	if (plyr->cheats & CF_GODMODE)
    {
		if (plyr->mo)
			plyr->mo->health = god_health;  // Ty 03/09/98 - deh
		
		plyr->health = god_health;
		plyr->message = s_STSTR_DQDON; // Ty 03/27/98 - externalized
    }
	else
		plyr->message = s_STSTR_DQDOFF; // Ty 03/27/98 - externalized
}

void ResetMaps_f() {
	playState.numMapStats = 0;
	memset( playState.mapStats, 0, sizeof( playState.mapStats ) );
}

/*
 ==================
 iphoneStartup
 
 ==================
 */
struct addrinfo *addrinfoHead;

void D_DoomMainSetup( const char * iwad, const char * pwad );
void iphoneStartup() {
	int		start = SysIphoneMilliseconds();
	
	// microseconds will be plenty random for playerID and localGameID
	playerID = localGameID = SysIphoneMicroseconds();
	
	InitImmediateModeGL();

	// init OpenAL before pak file, so the pak file can
	// make all the al static buffers
	Sound_Init();
	
	char buffer[1028];
	sprintf( buffer, "%s/base.iPack", SysIphoneGetAppDir() );
	// get our new-style pak file
	PK_Init( buffer );

	// register console commands
	Cmd_AddCommand( "listcvars", Cvar_List_f );
	Cmd_AddCommand( "resetcvars", Cvar_Reset_f );
	Cmd_AddCommand( "resetmaps", ResetMaps_f );
	Cmd_AddCommand( "listcmds", Cmd_ListCommands_f );
	Cmd_AddCommand( "give", Give_f );
	Cmd_AddCommand( "god", God_f );
	Cmd_AddCommand( "mail", EmailConsole );  //gsh, mails the console to id

	// register console variables
	Cvar_Get( "version", va( "%3.1f %s %s", DOOM_IPHONE_VERSION, __DATE__, __TIME__ ), 0 );
    
    freeLevelOfWeek = Cvar_Get("freeLevelOfWeek", "0", 0 );
	skill = Cvar_Get( "skill", "1", CVAR_ARCHIVE );
	episode = Cvar_Get( "episode", "0", CVAR_ARCHIVE );

	controlScheme = Cvar_Get( "controlScheme", "0", CVAR_ARCHIVE );
	stickTurn = Cvar_Get( "stickTurn", "128", CVAR_ARCHIVE );
	stickMove = Cvar_Get( "stickMove", "128", CVAR_ARCHIVE );
	stickDeadBand = Cvar_Get( "stickDeadBand", "0.05", CVAR_ARCHIVE );
	rotorTurn = Cvar_Get( "rotorTurn", "50000", CVAR_ARCHIVE );
	tiltTurn = Cvar_Get( "tiltTurn", "0", CVAR_ARCHIVE );
	tiltMove = Cvar_Get( "tiltMove", "0", CVAR_ARCHIVE );
	tiltDeadBand = Cvar_Get( "tiltDeadBand", "0.08", CVAR_ARCHIVE );
	tiltAverages = Cvar_Get( "tiltAverages", "3", CVAR_ARCHIVE );
	centerSticks = Cvar_Get( "centerSticks", "1", CVAR_ARCHIVE );
	rampTurn = Cvar_Get( "rampTurn", "1", CVAR_ARCHIVE );

	music = Cvar_Get( "music", "1", CVAR_ARCHIVE );
	cropSprites = Cvar_Get( "cropSprites", "1", 0 );
	mapScale = Cvar_Get( "mapScale", "10", CVAR_ARCHIVE );
	drawControls = Cvar_Get( "drawControls", "1", CVAR_ARCHIVE );
	autoUse = Cvar_Get( "autoUse", "1", CVAR_ARCHIVE );
	statusBar = Cvar_Get( "statusBar", "1", CVAR_ARCHIVE );
	touchClick = Cvar_Get( "touchClick", "1", CVAR_ARCHIVE );
	messages = Cvar_Get( "messages", "1", CVAR_ARCHIVE );
	mapSelectY = Cvar_Get( "mapSelectY", "0", CVAR_ARCHIVE );
	miniNet = Cvar_Get( "miniNet", "1", CVAR_ARCHIVE );

	// multiplayer setup
	timeLimit = Cvar_Get( "timeLimit", "0", CVAR_ARCHIVE );
	fragLimit = Cvar_Get( "fragLimit", "5", CVAR_ARCHIVE );
	mpDeathmatch = Cvar_Get( "mpDeathmatch", "0", CVAR_ARCHIVE );
	mpDataset = Cvar_Get( "mpDataset", "0", CVAR_ARCHIVE );
	mpEpisode = Cvar_Get( "mpEpisode", "1", CVAR_ARCHIVE );
	mpSkill = Cvar_Get( "mpSkill", "1", CVAR_ARCHIVE );
	mpMap = Cvar_Get( "mpMap", "1", CVAR_ARCHIVE );
	mpExpansion = Cvar_Get( "mpExpansion", "0", CVAR_ARCHIVE | CVAR_NOSET );
	
    // WADs to load
    iwadSelection = Cvar_Get( "iwadSelection", "doom.wad", CVAR_ARCHIVE );
    pwadSelection = Cvar_Get( "pwadSelection", "", CVAR_ARCHIVE );
    
	// debug tools
	showTilt = Cvar_Get( "showTilt", "-1", 0 );
	showTime = Cvar_Get( "showTime", "0", 0 );
	showNet = Cvar_Get( "showNet", "0", 0 );
	showSound = Cvar_Get( "showSound", "0", 0 );
	noBlend = Cvar_Get( "noBlend", "0", 0 );	// disable the damae blends for screenshots
	glfinish = Cvar_Get( "glfinish", "0", 0 );
	throttle = Cvar_Get( "throttle", "0", 0 );	// network packet throttle enable
	
	// Was origiinally 4. Trying different values to help internet play.
	netBuffer = Cvar_Get( "netBuffer", "12", 0 );	// max tics to buffer ahead
	
	// load the archived cvars
	Cmd_ExecuteFile( va( "%s/config.cfg", SysIphoneGetDocDir() ) );
	
    // Check if our WADs were bad last time.
    {
        FILE    *fp;
        char    path[1024];
        snprintf( path, sizeof( path ), "%s/abandon.ship", SysIphoneGetDocDir() );
        fp = fopen( path, "r" );
        if( fp ) {
            Com_Printf("Last exit was fatal (ship abandoned). Recovering...\n");

            Cvar_Set( "iwadSelection", "doom.wad" );
            Cvar_Set( "pwadSelection", "" );

            iwadSelection = Cvar_Get( "iwadSelection", "doom.wad", CVAR_ARCHIVE );
            pwadSelection = Cvar_Get( "pwadSelection", "", CVAR_ARCHIVE );
            
            // remove canary
            if( remove(path) != 0 ) {
                Com_Printf("Could not remove canary. This is bad!\n");
            }
        }
    }
    
	// start the intro music if it wasn't disabled with the music cvar
	iphonePlayMusic( "intro" );
//	iphonePlayMusic( "e1m1" );
	
	// these should get overwritten by the config loading
	memset( &playState, 0, sizeof( playState ) );
	playState.map.skill = 1;
	playState.map.episode = 1;
	playState.map.map = 1;
	HudSetForScheme( 0 );
	
	// load the binary config file
	FILE *f = fopen( va( "%s/binaryConfig.bin", SysIphoneGetDocDir() ), "rb" );
	if ( f ) {
		long int version;
		
		version = 0;
		fread( &version, 1, sizeof( version ), f );
		if ( version != VERSION_BCONFIG ) {
			Com_Printf( "Binary config file bad version.\n" );
		} else {
			fread( &playState, 1, sizeof( playState ), f );
            
            fread( &huds, 1, sizeof( huds ), f );

            //hud_t fakehuds;
            //fread( &fakehuds, 1, sizeof(fakehuds), f );

			version = 0;
			fread( &version, 1, sizeof( version ), f );
			if ( version != VERSION_BCONFIG ) {
				Com_Error( "Binary config file bad trailing version.\n" );
			}
		}
		fclose( f );
	}
	
	Com_Printf( "startup time: %i msec\n", SysIphoneMilliseconds() - start );

	start = SysIphoneMilliseconds();
	
	// the texnums might have been different in the savegame
	HudSetTexnums();
	
	arialFontTexture = PK_FindTexture( "iphone/arialImageLAL.tga" );
	
	Com_Printf( "preloadBeforePlay(): %i msec\n", SysIphoneMilliseconds() - start );	

	// prBoom seems to draw the static pic screens without setting up 2D, causing
	// a bad first frame
	iphoneSet2D();	
	
	menuState = IPM_MAIN;
    lastState = IPM_MAIN;
	
#if 0
	// jump right to the save spot for debugging
	ResumeGame();
#endif
}

/*
 ==================
 iphoneWadSelect
 
 Apply WAD file selection
 ==================
*/
void iphoneIWADSelect( const char* iwad ) {
    char full_iwad[1024];
    
    I_FindFile( iwad, ".wad", full_iwad );
    
    if( full_iwad[0] == '\0' ) {
        // fall back to vanilla Doom IWAD.
        
        I_FindFile( "doom.wad", ".wad", full_iwad );
        iwad = "doom.wad";
    }
    
    if( doom_iwad ) free(doom_iwad);
    
    // IWAD is part of bundle, do NOT store 
    doom_iwad = strdup( iwad );
}

/*
 ==================
 iphonePWADAdd
 
 Add a PWAD file selection
 ==================
 */
void iphonePWADAdd( const char* pwad  ) {
    
    char full_pwad[1024];
    
    I_FindFile( pwad, ".wad", full_pwad );
    
    if( full_pwad[0] != '\0' && strcmp( pwad, "" ) != 0 ) {
        // +2, 1 for separator and 1 for null terminator
        char* pwadlist = (char*)malloc( sizeof(char)*(strlen(doom_pwads)+strlen(full_pwad)+2) );
        strcpy( pwadlist, doom_pwads);
        strcat( pwadlist, full_pwad);
        unsigned long len = strlen(pwadlist);
        pwadlist[len] = PWAD_LIST_SEPARATOR;
        pwadlist[len+1] = '\0';
        //if( doom_pwads ) free(doom_pwads);
        doom_pwads = pwadlist;
        Com_Printf("Added PWAD: %s (%s)\n", pwad, doom_pwads);
    } else {
        Com_Printf("Failed to add PWAD: %s (%s)\n", pwad, doom_pwads);
    }
    
}

/*
 ==================
 iphonePWADRemove
 
 Remove a PWAD from the current selection
 ==================
 */
void iphonePWADRemove( const char* pwad  ) {
    char full_pwad[1024];
    
    I_FindFile( pwad, ".wad", full_pwad );
    
    char* pwadloc_start = strstr( doom_pwads, full_pwad );
    char* pwadloc_end = strchr( pwadloc_start, PWAD_LIST_SEPARATOR );
    
    // if this is the only PWAD...
    if( pwadloc_start == doom_pwads && pwadloc_end == (doom_pwads + (strlen(doom_pwads) - 1)) ) {
        if( doom_pwads ) free(doom_pwads);
        doom_pwads = strdup("");
        Com_Printf("Removed only PWAD: %s (%s)\n", pwad, doom_pwads);
    } else if( pwadloc_start && pwadloc_end && full_pwad[0] != '\0' && strcmp( pwad, "" ) != 0 ) {
    
        // remove pwad and its separator, hence +1
        unsigned long i = 0;
        unsigned long pwadlist_len = (strlen(doom_pwads)-(strlen(full_pwad)+1) );
        char* pwadlist = (char*)malloc( sizeof(char)*pwadlist_len );
        
        char* pwadlocal = doom_pwads;
        
        
        while( pwadlocal != pwadloc_start && i < pwadlist_len ) {
            pwadlist[i++] = *pwadlocal++;
        }
        
        pwadlocal = pwadloc_end+1; // advance past the separator
        while( *pwadlocal && i < pwadlist_len ) {
            pwadlist[i++] = *pwadlocal++;
        }
        
        pwadlist[i] = '\0';
        
        if( doom_pwads ) free(doom_pwads);
        doom_pwads = pwadlist;
        Com_Printf("Removed PWAD: %s (%s)\n", pwad, doom_pwads);
    } else {
        Com_Printf("Failed to remove PWAD: %s (%X,%X %s)\n", pwad,pwadloc_start,pwadloc_end,doom_pwads);
    }
}

/*
 ==================
 iphoneClearPWADs
 
 Remove all PWADs from the current selection
 ==================
 */
void iphoneClearPWADs() {
    //if( doom_pwads ) free(doom_pwads);
    doom_pwads = strdup("");
}

/*
 ==================
 iphoneSanitizePWADs
 
 Ensure all PWADs from the current selection actually exist.
 Useful when resuming from shutdown and checking for changed paths.
 ==================
 */
void iphoneSanitizePWADs() {
    if( strlen(doom_pwads) == 0 ) return;
    char* pwad_local = strdup(doom_pwads);
    if(!pwad_local) return;
    
    char* pwad_final = (char*)malloc( sizeof(char)*(1+strlen(doom_pwads)));
    if(!pwad_final) {
        free(pwad_local);
        return;
    }
    
    pwad_final[0] = '\0'; // start with blank string
    char delim[2] = { PWAD_LIST_SEPARATOR, '\0' };
    char* pwad = strtok(pwad_local, delim );
    
    while( pwad != NULL ) {
        char pwad_path[1024] = { 0 };
        char pwad_doc_path[1024];
        char* pwad_file = strrchr(pwad,'/');
        
        if( pwad_file && *pwad_file ) {
            pwad_file++;
            strcpy( pwad_doc_path, SysIphoneGetDocDir() );
            strcat( pwad_doc_path, "/" );
            strcat( pwad_doc_path, pwad_file );
            I_FindFile(pwad_doc_path,".wad",pwad_path);
            
            // try the app dir
            if( !*pwad_path ) {
                strcpy( pwad_doc_path, SysIphoneGetAppDir() );
                strcat( pwad_doc_path, "/" );
                strcat( pwad_doc_path, pwad_file );
                I_FindFile(pwad_doc_path,".wad",pwad_path);
            }
        }
        
        if( *pwad_path ) {
            // keep this pwad
            strcat(pwad_final, pwad_path);
            strcat(pwad_final, delim);
            Com_Printf("PWAD Retained: %s\n", pwad);
        } else {
            Com_Printf("PWAD Lost: %s\n", pwad);
        }
        
        pwad = strtok( NULL, delim );
    }
   
    if( doom_pwads ) free(doom_pwads);
    doom_pwads = pwad_final;
    free(pwad_local);
}

/*
 ==================
 iphoneDoomSetup
 
 Run the Doom game setup functions. This was made seperate from iphoneStartup so that the user
 could select a mission pack first.
 ==================
 */
void iphoneDoomStartup() {
	Com_Printf( "---------- D_DoomMain ----------\n" );
    
    char full_iwad[1024];
    
    I_FindFile( doom_iwad, ".wad", full_iwad );
    
    if( full_iwad[0] == '\0' ) {
        // fall back to vanilla Doom IWAD.
        
        I_FindFile( "doom.wad", ".wad", full_iwad );
    }
    
    iphoneSanitizePWADs();
    
	D_DoomMainSetup( full_iwad, doom_pwads );
    
    // upon successful setup, save CVARs for future use
    // ensure we never save a NULL string (this should never happen)
    if( doom_pwads == NULL ) {
        doom_pwads = strdup("");
    }
    
    Cvar_Set( "iwadSelection", doom_iwad );
    Cvar_Set( "pwadSelection", doom_pwads );
	
	// put savegames here
    strcpy( basesavegame, SysIphoneGetDocDir() );
	
}

/*
 ==================
 iphoneShutdown
 
 Write out configs and save the game at this position
 ==================
 */
void iphoneShutdown() {
	FILE	*fp;
	char	path[1024];
	cvar_t	*var;
	char	buffer[1024];
    
    if( lastState == IPM_GAME && gamestate != GS_INTERMISSION && !demoplayback ) {
      G_DoSaveGame( false );
    }
    
	// write the ascii config file
	snprintf( path, sizeof( path ), "%s/config.cfg", SysIphoneGetDocDir() );
	fp = fopen( path, "w" );
	if( ! fp ) {
		Com_Printf( "Could not write config.cfg.\n" );
		return;
	}
	
	// write out commands to set the archived cvars
	for( var = cvar_vars ; var ; var = var->next ) {
		if( var->flags & CVAR_ARCHIVE ) {
			snprintf( buffer, sizeof( buffer ), "%s %s\n", var->name, var->string );
			fprintf( fp, "%s", buffer );
			Com_Printf( "%s", buffer );
		}
	}
	
	fclose( fp );
	

	// write the binary config file
	FILE *f = fopen( va( "%s/binaryConfig.bin", SysIphoneGetDocDir() ), "wb" );
	if ( !f ) {
		Com_Printf( "Could not write binaryConfig.cfg.\n" );
		return;
	}
	
	long int version = VERSION_BCONFIG;
	
	fwrite( &version, 1, sizeof( version ), f );
	
	fwrite( &playState, 1, sizeof( playState ), f );
	fwrite( &huds, 1, sizeof( huds ), f );
	
	fwrite( &version, 1, sizeof( version ), f );
	fclose( f );
	
}


