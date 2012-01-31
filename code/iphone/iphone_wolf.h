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

// define this to get only the first episode on selections, and the
// automatic sell screen at the end of episode 1
#define EPISODE_ONE_ONLY

// this is the version number displayed on the menu screen
#define DOOM_IPHONE_VERSION 0.1

// if defined, the game runs in a separate thread from the app event loop
#define	USE_GAME_THREAD

typedef enum menuState {
	IPM_GAME,
	IPM_MAIN,
	IPM_MAPS,
	IPM_MULTIPLAYER,
	IPM_MASTER,
	IPM_CONTROLS,
	IPM_OPTIONS,
	IPM_HUDEDIT
} menuState_t;

extern menuState_t menuState;

void iphoneDrawMenus();

#define	VID_WIDTH	480
#define VID_HEIGHT	320

#define MAX_SKILLS		5
#define MAX_MAPS		200

#define MF_TRIED		1
#define MF_COMPLETED	2
#define MF_KILLS		4
#define MF_SECRETS		8
#define MF_TREASURE		16
#define MF_TIME			32

// we want to track mapStats for downloaded content, so we
// won't have a known number of these
typedef struct {
	int		dataset;
	int		episode;
	int		map;
	
	int		completionFlags[MAX_SKILLS];
} mapStats_t;
	
// this structure is saved out at the head of the binary save file,
// and allows all the menus to work without having to load a game save
typedef struct {
	int		version;

	int		episode;
	int		map;
	int		skill;
	
	int		mapsStarted;	// when 0, resume game will just be a new game
	
	// if someone downloads more than MAX_MAPS, they won't get stat tracking on them.
	int		numMapStats;
	mapStats_t	mapStats[MAX_MAPS];
} currentMap_t;

extern currentMap_t currentMap;

extern boolean	levelHasBeenLoaded;	// determines if "resume game" does a loadGame and exiting does a saveGame

extern pkTexture_t *fontTexture;
extern pkTexture_t *numberPics[10];

extern int		iphoneFrameNum;
extern int		levelLoadFrameNum;
extern int		consoleActive;

extern cvar_t	*skill;
extern cvar_t	*episode;
extern cvar_t	*controlScheme;
extern cvar_t	*stickMove;
extern cvar_t	*stickTurn;
extern cvar_t	*rotorTurn;
extern cvar_t	*stickDeadBand;
extern cvar_t	*tiltTurn;
extern cvar_t	*tiltMove;
extern cvar_t	*tiltDeadBand;
extern cvar_t	*tiltAverages;
extern cvar_t	*music;
extern cvar_t	*showTilt;
extern cvar_t	*showTime;
extern cvar_t	*cropSprites;
extern cvar_t	*revLand;
extern cvar_t	*mapScale;
extern cvar_t	*hideControls;
extern cvar_t	*tapFire;
extern cvar_t	*skipSleep;
extern cvar_t	*autoUse;
extern cvar_t	*statusBar;
extern cvar_t	*touchClick;

extern int	numTouches;
extern int	touches[5][2];	// [0] = x, [1] = y in landscape mode, raster order with y = 0 at top
// so we can detect button releases
extern int	numPrevTouches;
extern int prevTouches[5][2];

extern float	tilt;		// -1.0 to 1.0
extern float	tiltPitch;

extern	boolean	drawWeaponSelect;			// true when the weapon select overlay is up
extern	int		weaponSelected;				// -1 for no change

typedef unsigned char color4_t[4];
typedef unsigned char color3_t[3];

typedef struct {
	int		enterFrame;
	int		beforeSwap;
	int		afterSwap;
	int		afterSleep;
} logTime_t;
#define MAX_LOGGED_TIMES	512
extern logTime_t	loggedTimes[MAX_LOGGED_TIMES];	// indexed by iphoneFrameNum

void LoadWallTexture( int wallPicNum );

int	TouchDown( int x, int y, int w, int h );
int	TouchReleased( int x, int y, int w, int h );
int iphoneDrawText( int x, int y, const char *str );
int iphoneCenterText( int x, int y, const char *str );
void iphoneDrawNumber( int x, int y, int number, int charWidth, int charHeight );
void iphoneDrawPic( int x, int y, int w, int h, const char *pic );
int iphoneDrawPicWithTouch( int x, int y, int w, int h, const char *pic );
void StartGame();
void iphoneOpenAutomap();
void iphoneDrawNotifyText();
void iphoneSet2D( void );
bool TextButton( const char *title, int x, int y, int w, int h );

void R_Draw_Fill( int x, int y, int w, int h, color3_t c );
void R_Draw_Blend( int x, int y, int w, int h, color4_t c );

void InitImmediateModeGL();
int iphoneRotateForLandscape();
void iphoneCheckForLandscapeReverse();

void iphonePacifierUpdate();
void iphoneDrawScreen();
	
extern int damageflash;
extern int bonusFrameNum;
extern int attackDirTime[2];


#define HF_DISABLED				1
#define HF_NODRAW				2		// invisible button

typedef struct {
	int		x, y;						// mdpoint
	int		drawWidth, drawHeight;
	int		touchWidth, touchHeight;	// allow touches outside the actual bounds
	pkTexture_t	*texture;
	boolean	drawAsLimit;				// draw with red tint to show further movement won't do anything
	float	touchState;
	float	drawState;					// offsets for rotors
	int		hudFlags;
	struct touch_s	*touch;
} hudPic_t;

typedef struct {
	hudPic_t	forwardStick;
	hudPic_t	sideStick;
	hudPic_t	turnStick;
	hudPic_t	turnRotor;
	hudPic_t	fire;
	hudPic_t	menu;
	hudPic_t	map;
	hudPic_t	weaponSelect;
} hud_t;

extern hud_t	huds;

void HudSetForScheme( int schemeNum );
void HudSetTexnums();
void HudEditFrame();

void Sound_StartLocalSound( const char *sound );
void Sound_StartLocalSoundAtVolume( const char *sound, float volume );

int BackButton();
void ResumeGame();

//---------------------------------------
// Touch and button
//---------------------------------------

typedef struct touch_s {
	boolean	down;
	int		x, y;
	int		prevX, prevY;	// will be set to x, y on first touch, copied after each game frame
	int		stateCount;		// set to 1 on first event that state changes, incremented each game frame
	void	*controlOwner;
	void	*identification;
} touch_t;

#define	MAX_TOUCHES		5
extern touch_t		sysTouches[MAX_TOUCHES];
extern touch_t		gameTouches[MAX_TOUCHES];
extern pthread_mutex_t	eventMutex;		// used to sync between game and event threads

touch_t *TapInBounds( int x, int y, int w, int h );
touch_t *TouchInBounds( int x, int y, int w, int h );
touch_t *UpdateHudTouch( hudPic_t *hud );

typedef struct {
	pkTexture_t *texture;
	const char	*title;
	int			x, y, w, h;
	touch_t		*touch;
	float		scale;			// ramps up and down after touches
	int			frameNum;		// reset scale if not checked on previous frame
} ibutton_t;

void SetButtonPics( ibutton_t *button, const char *picName, const char *title, int x, int y );
void SetButtonText( ibutton_t *button, const char *title, int x, int y, int w, int h );
boolean HandleButton( ibutton_t *button );


//---------------------------------------
// Doom stuff we call directly
//---------------------------------------
void G_DoSaveGame (boolean menu);

//---------------------------------------
// iphone_mapSelect.c
//---------------------------------------
void DisplayLoadingScreen();
void iphoneMapSelectMenu();

//---------------------------------------
// interfaces from the original game code
//---------------------------------------
void iphoneSetNotifyText( const char *str, ... );

void iphoneIntermission( wbstartstruct_t* wbstartstruct );

//---------------------------------------
// interfaces to Objective-C land
//---------------------------------------

// The event thread will fill this after hitting enter
// on the console.  The game thread should check it,
// execute it, and clear it under mutex.
extern char	consoleCommand[1024];

void SysIPhoneSwapBuffers();
void SysIPhoneVibrate();
void SysIPhoneOpenURL( const char *url );
void SysIPhoneSetUIKitOrientation( int isLandscapeRight );
const char * SysIPhoneGetConsoleTextField();
void SysIPhoneSetConsoleTextField(const char *);
void SysIPhoneInitAudioSession();
int SysIPhoneOtherAudioIsPlaying();
int SysIphoneMilliseconds();
int SysIphoneMicroseconds();
const char * SysIphoneGetAppDir();
const char * SysIphoneGetDocDir();

//---------------------------------------
// interfaces from Objective-C land
//---------------------------------------
void iphoneStartup();
void iphoneShutdown();
void iphoneFrame();
void iphoneTiltEvent( float *tilts );
void iphoneTouchEvent( int numTouches, int touches[16] );
void iphoneActivateConsole();
void iphoneDeactivateConsole();
void iphoneExecuteCommandLine();

