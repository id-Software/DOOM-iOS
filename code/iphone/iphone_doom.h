/*
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
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

// this is the version number displayed on the menu screen
#define DOOM_IPHONE_VERSION 0.9

// if defined, the game runs in a separate thread from the app event loop
#define	USE_GAME_THREAD

typedef enum menuState {
	IPM_GAME,
	IPM_MAIN,
	IPM_MAPS,
	IPM_MULTIPLAYER,
	IPM_CONTROLS,
	IPM_OPTIONS,
	IPM_HUDEDIT,
	IPM_PACKET_TEST
} menuState_t;

extern menuState_t menuState;
extern menuState_t lastState;

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

// the level select screen returns this
typedef struct {
	int		dataset;
	int		episode;
	int		map;
	int		skill;	
} mapStart_t;

// this structure is saved out at the head of the binary save file,
// and allows all the menus to work without having to load a game save
typedef struct {
	mapStart_t	map;			// this is the map currently being run
	
	int		saveGameIsValid;	// when 0, resume game will just be a new game
	
	// if someone downloads more than MAX_MAPS, they won't get stat tracking on them.
	int		numMapStats;
	mapStats_t	mapStats[MAX_MAPS];
} playState_t;

extern playState_t playState;

extern boolean	levelHasBeenLoaded;	// determines if "resume game" does a loadGame and exiting does a saveGame

extern pkTexture_t *arialFontTexture;

// set to 1 when app is exiting to cause game thread to do a save game,
// which would not be safe to do from the event thread
extern volatile int		saveOnExitState;

extern int		asyncTicNum;			// 30hz
extern int		iphoneFrameNum;			// frame rate dependent, max of 30hz
extern int		levelLoadFrameNum;
extern int		consoleActive;
extern boolean	iphoneTimeDemo;
extern int		timeDemoStart;
extern char		timeDemoResultString[80];

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
extern cvar_t	*miniNet;
extern cvar_t	*showTilt;
extern cvar_t	*showTime;
extern cvar_t	*showNet;
extern cvar_t	*showSound;
extern cvar_t	*cropSprites;
extern cvar_t	*revLand;
extern cvar_t	*mapScale;
extern cvar_t	*drawControls;
extern cvar_t	*autoUse;
extern cvar_t	*statusBar;
extern cvar_t	*touchClick;
extern cvar_t	*messages;
extern cvar_t	*timeLimit;
extern cvar_t	*fragLimit;
extern cvar_t	*mpDeathmatch;
extern cvar_t	*mpDataset;
extern cvar_t	*mpSkill;
extern cvar_t	*mpEpisode;
extern cvar_t	*mpMap;
extern cvar_t	*glfinish;
extern cvar_t	*mapSelectY;
extern cvar_t	*throttle;
extern cvar_t	*centerSticks;
extern cvar_t	*rampTurn;
extern cvar_t	*netBuffer;

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

// networking
enum {
	PACKET_VERSION_BASE = 0x24350010,
	PACKET_VERSION_SETUP,
	PACKET_VERSION_JOIN,
	PACKET_VERSION_CLIENT,
	PACKET_VERSION_SERVER
} packetType_t;

#define DOOM_PORT	14666		// setup packets will go to DOOM_PORT+1

// the server sends out a setup packet by broadcast, and also directly addressed
// to each client that has joined the game because broadcast packets have truly
// crappy delivery characteristics over WiFi
typedef struct {
	int		packetType;
	int		gameID;		// every change to anything in packetSetup_t must change gameID
	int		startGame;	// when this is set, start running the game
	
	int		sendCount;	// just for packet drop tests
	
	mapStart_t	map;
	
	int		deathmatch;
	int		fraglimit;
	int		timelimit;
	
	int		playerID[MAXPLAYERS];	// 0 = not in game
} packetSetup_t;

// If we have received a recent setup packet before hitting the multiplayer
// button, we will send a join packet to that server.  Otherwise, we will
// start acting as a new server.
typedef struct {
	int		packetType;
	
	// this should match the packetSetup.gameID
	int		gameID;
	
	int		playerID;
} packetJoin_t;

typedef struct {
	int		packetType;
	
	// gameID is determined randomly during setup, any packet that doesn't
	// match is discarded
	int		gameID;

	// this could be used to tell when we are dropping client packets
	// but it isn't critical
	int		packetSequence;
	
	// used to show current round trip latency
	int		packetAcknowledge;
	
	// the client's clock at the time the packet was sent, used
	// to track one-way latency
	int		milliseconds;
	
	// the server could match this up based on ip alone, but it is nice to have
	int		consoleplayer;

	// the last tic that the client has run
	int		gametic;
	
	// some commands will get missed over the network
	ticcmd_t	cmd;
} packetClient_t;

typedef struct {
	int		packetType;
	
	// gameID is determined randomly during setup, any packet that doesn't
	// match is discarded
	int		gameID;

	// used to detect packet drops
	int		packetSequence;
	
	// used to show current round trip latency
	int		packetAcknowledge;
	
	// the server's clock at the time the packet was sent, used
	// to track one-way latency
	int		milliseconds;
	
	// consistancyTic will be the last acknowledged gametic for this
	// particular client
	int		consistancyTic;

	// constancy is used to see if somehow the game running on the
	// client has diverged from the one running on the server,
	// which is an unrecoverable error
	short	consistancy[MAXPLAYERS];
	
	// this will be the last pc.gametic from the player
	int		starttic;
	
	// netcmds[][(maketic-1)&BACKUPTICMASK] is the most recent
	int		maketic;
	
	// only the [playersInGame*(maketic-starttic)] will be transmitted
	ticcmd_t	netcmds[MAXPLAYERS*BACKUPTICS];
} packetServer_t;

extern int		gameSocket;
extern	struct sockaddr_in gameSocketAddress;

extern int		playerID;
extern int		gameID;
extern int		localGameID;
extern int		packetSequence;

// Only one game can be set up at a time on a given wireless segment, although
// several independent games can be played.
// If a valid setupPacket has arrived in the last second, that will be the
// displayed game, otherwise the local system starts sending out setupPackets.
extern packetSetup_t	setupPacket;
extern int				setupPacketFrameNum;
extern int				localGameID;	// change every time we take over as the sender of setupPackets

// set after each game tic if a usable line is in front of the player
extern boolean	autoUseActive;
extern boolean respawnActive;

typedef enum {
	NF_NONE,
	NF_CONSISTANCY,
	NF_INTERRUPTED
} netFail_t;
extern netFail_t netGameFailure;				// set by asyncThread

typedef struct {
	int		interfaceIndex;			// we must use the right socket to send packets
	struct sockaddr	address;
	int		oneWayLatency;			// will always have 30+ msec of jitter
	int		lastPacketAsyncTic;		// to easily tell if it just arrived
	int		lastPacketTime;			// local milliseconds of last receive	
	int		lastTimeDelta;			// packet milliseconds - local milliseconds
	int		lowestTimeDelta;		// min'd with lastTimeDelta each arrival
	int		currentPingTics;		// packetSequence - last packetAcknowledge
} netPeer_t;

typedef struct {
	netPeer_t		peer;
	packetClient_t	pc;				// most recent packet received
	
	// TODO: There would be some benefit to ensuring that no edge transitions on
	// buttons are missed due to clock/net jitter.
} netPlayer_t;

// all received packets, whether bluetooth or WiFi, go through here
void iphoneProcessPacket( const struct sockaddr *from, const void *data, int len );

extern netPeer_t	netServer;
extern netPlayer_t	netPlayers[MAXPLAYERS];
extern sem_t *		ticSemaphore;

typedef struct {
	int		numGameTics;
	int		numPingTics;
	int		enterFrame;
	int		afterSleep;
	int		beforeSwap;
	int		afterSwap;
} logTime_t;
#define MAX_LOGGED_TIMES	512
extern logTime_t	loggedTimes[MAX_LOGGED_TIMES];	// indexed by iphoneFrameNum

void LoadWallTexture( int wallPicNum );

float	StringFontWidth( const char *str );
int	TouchDown( int x, int y, int w, int h );
int	TouchReleased( int x, int y, int w, int h );
int	TouchPressed( int x, int y, int w, int h );

// y is the baseline for font drawing
float iphoneDrawText( float x, float y, float scale, const char *str );
float iphoneCenterText( float x, float y, float scale, const char *str );

void StartGame();
void iphoneOpenAutomap();
void iphoneDrawNotifyText();
void iphoneSet2D( void );

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

#define BF_IGNORE		1		// don't draw or process touches
#define BF_INACTIVE		2		// draw, but no touch processing at all
#define BF_GLOW			4		// animated overbright glow
#define BF_DIMMED		8		// draw darker, but still selectable
#define BF_CENTERTEXT	16		// text in middle of button, not underneath
#define BF_TRANSPARENT	32		// blend translucent
#define BF_HUDBUTTON	64		// don't process in UpdateHudTouch
#define BF_DRAW_ACTIVE	128		// for fire button
#define BF_SMALL_CLICK	256		// for fire button

typedef struct {
	int			x, y;
	int			drawWidth, drawHeight;
	pkTexture_t *texture;
	
	const char	*title;
	struct touch_s *touch;
	float		scale;					// ramps up and down after touches
	int			frameNum;				// reset scale if not checked on previous frame
	int			buttonFlags;
	boolean		twoFingerPress;			// if a second finger came down before a release for timedemo / etc
	boolean		pressed;				// true when a touch goes down in it
	
	// stuff for hud controls
	boolean		drawAsLimit;			// color tint when further movement won't do anything
	float		touchState;				// rotor angle
	float		drawState;				// offsets for rotors
	int			downX, downY;			// initial touch went down here
} ibutton_t;

typedef struct {
	ibutton_t	forwardStick;
	ibutton_t	sideStick;
	ibutton_t	turnStick;
	ibutton_t	turnRotor;
	ibutton_t	fire;
	ibutton_t	menu;
	ibutton_t	map;
	ibutton_t	weaponSelect;
} hud_t;

extern hud_t	huds;

void HudSetForScheme( int schemeNum );
void HudSetTexnums();
void HudEditFrame();

boolean StartNetGame();

int BackButton();
void ResumeGame();

//---------------------------------------
// Touch and button
//---------------------------------------

typedef struct touch_s {
	boolean	down;
	int		x, y;
//	int		prevX, prevY;	// will be set to x, y on first touch, copied after each game frame
	int		stateCount;		// set to 1 on first event that state changes, incremented each game frame (-1 is a special tapped-and-released code)
	void	*controlOwner;
} touch_t;

#define	MAX_TOUCHES		5
extern touch_t		sysTouches[MAX_TOUCHES];
extern touch_t		gameTouches[MAX_TOUCHES];
extern pthread_mutex_t	eventMutex;		// used to sync between game and event threads

touch_t *TouchInBounds( int x, int y, int w, int h );
touch_t *AnyTouchInBounds( int x, int y, int w, int h );
touch_t *UpdateHudTouch( ibutton_t *hud );

bool NewTextButton( ibutton_t *b,  const char *title, int x, int y, int w, int h );
void SetButtonPics( ibutton_t *button, const char *picName, const char *title, int x, int y );
void SetButtonPicsAndSizes( ibutton_t *button, const char *picBase, const char *title, int x, int y, int w, int h );
boolean HandleButton( ibutton_t *button );

//---------------------------------------
// Doom stuff we use directly
//---------------------------------------
void G_DoSaveGame (boolean menu);
extern short    consistancy[MAXPLAYERS][BACKUPTICS];
extern boolean  levelTimer;
extern int      levelTimeCount;
extern boolean  levelFragLimit;
extern int      levelFragLimitCount;

//---------------------------------------
// iphone_sound.c
//---------------------------------------

void Sound_Init( void );
void Sound_StartLocalSound( const char *sound );
void Sound_StartLocalSoundAtVolume( const char *sound, float volume );

void ShowSound();

//---------------------------------------
// iphone_net.c
//---------------------------------------

// dump all the interfaces and ip addresses for debugging
void ReportNetworkInterfaces();

// open a UDP socket, pass "en0" for wifi
int	UDPSocket( const char *interfaceName, int portnum );

// return false if the multiplayer button should be disabled
boolean NetworkAvailable();

// this can be called every frame in the menu to highlight
// the multiplayer icon when a server is already up
boolean NetworkServerAvailable();

// returns "WiFi", "BlueTooth", or "" for display on the
// main menu multiplayer icon
const char *NetworkServerTransport();

// this queries DNS for the actual address
boolean ResolveNetworkServer( struct sockaddr *addr );

// If we are starting a server instead of joining one, make
// us available as a bonjour service until we start the game
// or back out of the multiplayer menu.  Returns false if
// someone else grabbed it just before we could.
boolean RegisterGameService();
void TerminateGameService();

// called by AsyncTic() to check for server state changes,
// registers for service browsing on first call.
void ProcessDNSMessages();

// draw a graph of packets sent and received
void ShowNet();
void ShowMiniNet();

//---------------------------------------
// iphone_mapSelect.c
//---------------------------------------

// returns false if nothing was selected
// if map->map is -1, the back button was hit instead of choosing a level
boolean iphoneMapSelectMenu( mapStart_t *map );

mapStats_t *FindMapStats( int dataset, int episode, int map, boolean create );
const char *FindMapName( int dataset, int episode, int map );

//---------------------------------------
// iphone_start.c
//
// game harness routines
//---------------------------------------
void ResumeGame();
boolean StartNetGame();
void StartSaveGame();
void StartSinglePlayerGame( mapStart_t	map );
void StartDemoGame( boolean timeDemoMode );

//---------------------------------------
// interfaces from the original game code
//---------------------------------------
void iphoneSetNotifyText( const char *str, ... );
void iphoneIntermission( wbstartstruct_t* wbstartstruct );
void iphoneStartLevel();
void iphoneStartMusic();
void iphoneStopMusic();
void iphonePlayMusic( const char *name );
void iphonePauseMusic();
void iphoneResumeMusic();
	
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
void iphoneAsyncTic();
void iphoneTiltEvent( float *tilts );

