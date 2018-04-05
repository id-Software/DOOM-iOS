/*
 *  prboomInterface.c
 *  doom
 *
 *  Created by John Carmack on 4/14/09.
 *  Copyright 2009 Id Software. All rights reserved.
 *
 * Stuff to get prboom to compile without SDL
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

int	desired_fullscreen;
int usejoystick;
int	joyright;
int	joyleft;
int joydown;
int	joyup;
int gl_colorbuffer_bits;
int gl_depthbuffer_bits;
int snd_card;
int mus_card;
int endoom_mode;
int use_fullscreen;
int	snd_samplerate;
int ms_to_next_tick;
int realtic_clock_rate;


/* I_SafeExit
 * This function is called instead of exit() by functions that might be called
 * during the exit process (i.e. after exit() has already been called)
 * Prevent infinitely recursive exits -- killough
 *
 * JDC: we don't do any atexit() calls on iphone, so this shouldn't be necessary
 */
void I_SafeExit(int rc) {
	static int has_exited;
	if (!has_exited) {
		has_exited=rc ? 2 : 1;
		exit(rc);
    }
}

void I_uSleep( unsigned long usec ) {
	usleep( (unsigned int) usec );
}

/*
 * HasTrailingSlash
 *
 * cphipps - simple test for trailing slash on dir names
 */

boolean HasTrailingSlash(const char* dn)
{
	return ( (dn[strlen(dn)-1] == '/') );
}

void I_FindFile(const char* wfname, const char* ext, char * returnFileName )
{
    sprintf( returnFileName, "%s/%s", SysIphoneGetAppDir(), wfname );
	if (access(returnFileName,F_OK))
		strcat(returnFileName, ext);	// try adding the extension
	if (!access(returnFileName,F_OK)) {
		lprintf(LO_INFO, " found %s\n", returnFileName);
        
        // Found the file.
        return;
	}
    
    // JDS: try assuming it's a full path instead
    sprintf( returnFileName, "%s", wfname );
    if (access(returnFileName,F_OK))
        strcat(returnFileName, ext);    // try adding the extension
    if (!access(returnFileName,F_OK)) {
        lprintf(LO_INFO, " found %s\n", returnFileName);
        
        // Found the file.
        return;
    }
    
    // did not find the file.
    returnFileName[0] = '\0';
    lprintf(LO_INFO, " NOT found %s\n", wfname );
    
#if 0	
	// lookup table of directories to search
	static const struct {
		const char *dir; // directory
		const char *sub; // subdirectory
		const char *env; // environment variable
		const char *(*func)(void); // for I_DoomExeDir
	} search[] = {
		{NULL}, // current working directory
		{NULL, NULL, "DOOMWADDIR"}, // run-time $DOOMWADDIR
		{DOOMWADDIR}, // build-time configured DOOMWADDIR
		{NULL, "doom", "HOME"}, // ~/doom
		{NULL, NULL, "HOME"}, // ~
		{NULL, NULL, NULL, I_DoomExeDir}, // config directory
		{"/usr/local/share/games/doom"},
		{"/usr/share/games/doom"},
		{"/usr/local/share/doom"},
		{"/usr/share/doom"},
	};
	
	int   i;
	/* Precalculate a length we will need in the loop */
	size_t  pl = strlen(wfname) + strlen(ext) + 4;
	
	for (i = 0; i < sizeof(search)/sizeof(*search); i++) {
		char  * p;
		const char  * d = NULL;
		const char  * s = NULL;
		/* Each entry in the switch sets d to the directory to look in,
		 * and optionally s to a subdirectory of d */
		// switch replaced with lookup table
		if (search[i].env) {
			if (!(d = getenv(search[i].env)))
				continue;
		} else if (search[i].func)
			d = search[i].func();
		else
			d = search[i].dir;
		s = search[i].sub;
		
		p = malloc((d ? strlen(d) : 0) + (s ? strlen(s) : 0) + pl);
		sprintf(p, "%s%s%s%s%s", d ? d : "", (d && !HasTrailingSlash(d)) ? "/" : "",
				s ? s : "", (s && !HasTrailingSlash(s)) ? "/" : "",
				wfname);
		
		if (access(p,F_OK))
			strcat(p, ext);
		if (!access(p,F_OK)) {
			lprintf(LO_INFO, " found %s\n", p);
			return p;
		}
		free(p);
	}

	return NULL;
#endif	
}



boolean I_StartDisplay(void) { 
	return true;
}

void I_EndDisplay(void) {}
int I_GetTime_RealTime(void) { return 0; }
fixed_t I_GetTimeFrac (void) { return 0; }
void I_GetTime_SaveMS(void) {}
unsigned long I_GetRandomTimeSeed(void) { return 0; }

//const char* I_GetVersionString(char* buf, size_t sz);
//const char* I_SigString(char* buf, size_t sz, int signum);

const char *I_DoomExeDir(void) { return SysIphoneGetAppDir(); }

//void I_SetAffinityMask(void);


/* 
 * I_Read
 *
 * cph 2001/11/18 - wrapper for read(2) which handles partial reads and aborts
 * on error.
 */
void I_Read(int fd, void* vbuf, size_t sz)
{
	unsigned char* buf = vbuf;
	
	while (sz) {
		int rc = (int) read(fd,buf,sz);
		if (rc <= 0) {
			I_Error("I_Read: read failed: %s", rc ? strerror(errno) : "EOF");
		}
		sz -= rc; buf += rc;
	}
}

/*
 * I_Filelength
 *
 * Return length of an open file.
 */

int I_Filelength(int handle)
{
	struct stat   fileinfo;
	if (fstat(handle,&fileinfo) == -1)
		I_Error("I_Filelength: %s",strerror(errno));
	return (int)fileinfo.st_size;
}





void I_PreInitGraphics(void){}
void I_CalculateRes(unsigned int width, unsigned int height){ (void)width; (void)height; }
void I_ShutdownGraphics(void){}
void I_SetPalette(int pal){ (void)pal; }
void I_UpdateNoBlit (void){}
void I_FinishUpdate (void){}
int I_ScreenShot (const char *fname){ (void)fname; return 0;}


// CPhipps -
// I_SetRes
// Sets the screen resolution
void I_SetRes(void)
{
	int i;
	
	I_CalculateRes(SCREENWIDTH, SCREENHEIGHT);
	
	// set first three to standard values
	for (i=0; i<3; i++) {
		screens[i].width = SCREENWIDTH;
		screens[i].height = SCREENHEIGHT;
		screens[i].byte_pitch = SCREENPITCH;
		screens[i].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
		screens[i].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
	}
	
	// statusbar
	screens[4].width = SCREENWIDTH;
	screens[4].height = (ST_SCALED_HEIGHT+1);
	screens[4].byte_pitch = SCREENPITCH;
	screens[4].short_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE16);
	screens[4].int_pitch = SCREENPITCH / V_GetModePixelDepth(VID_MODE32);
	
	lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_UpdateVideoMode(void)
{
	lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d\n", SCREENWIDTH, SCREENHEIGHT );
	
	V_InitMode(VID_MODEGL);
	I_SetRes();
#if 0	
	screens[0].not_on_heap = true;
	screens[0].data = NULL;
	screens[0].byte_pitch = screen->pitch;
	screens[0].short_pitch = screen->pitch / V_GetModePixelDepth(VID_MODE16);
	screens[0].int_pitch = screen->pitch / V_GetModePixelDepth(VID_MODE32);
#endif
	
	V_AllocScreens();
	
	R_InitBuffer(SCREENWIDTH, SCREENHEIGHT);
	gld_Init(SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
	char titlebuffer[2048];
	static int    firsttime=1;

	SCREENWIDTH = displaywidth;
	SCREENHEIGHT = displayheight;
	
	if (firsttime)
	{
		firsttime = 0;
		
		atexit(I_ShutdownGraphics);
		lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
		
		/* Set the video mode */
		I_UpdateVideoMode();
		
		/* Setup the window title */
		strcpy(titlebuffer,PACKAGE);
		strcat(titlebuffer," ");
		strcat(titlebuffer,VERSION);
//		SDL_WM_SetCaption(titlebuffer, titlebuffer);
		
		/* Initialize the input system */
//		I_InitInputs();
	}
}


/* I_StartTic
 * Called by D_DoomLoop,
 * called before processing each tic in a frame.
 * Quick syncronous operations are performed here.
 * Can call D_PostEvent.
 */
void I_StartTic (void){}

/* I_StartFrame
 * Called by D_DoomLoop,
 * called before processing any tics in a frame
 * (just after displaying a frame).
 * Time consuming syncronous operations
 * are performed here (joystick reading).
 * Can call D_PostEvent.
 */

void I_StartFrame (void){}


void I_Init() {
	I_InitSound();
}

unsigned int SDL_GetTicks() { return 0; }

int (*I_GetTime)(void) = I_GetTime_RealTime;

