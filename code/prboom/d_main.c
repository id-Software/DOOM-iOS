/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2004 by
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
 *  DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
 *  plus functions to determine game mode (shareware, registered),
 *  parse command line parameters, configure game parameters (turbo),
 *  and call the startup functions.
 *
 *-----------------------------------------------------------------------------
 */


#ifdef _MSC_VER
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "doomdef.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_net.h"
#include "dstrings.h"
#include "sounds.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_checksum.h"
#include "i_main.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_fps.h"
#include "d_main.h"
#include "d_deh.h"  // Ty 04/08/98 - Externalizations
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf
#include "am_map.h"
#include "doomiphone.h"

void GetFirstMap(int *ep, int *map); // Ty 08/29/98 - add "-warp x" functionality
static void D_PageDrawer(void);

// CPhipps - removed wadfiles[] stuff

boolean devparm;        // started game with -devparm

// jff 1/24/98 add new versions of these variables to remember command line
boolean clnomonsters;   // checkparm of -nomonsters
boolean clrespawnparm;  // checkparm of -respawn
boolean clfastparm;     // checkparm of -fast
// jff 1/24/98 end definition of command line version of play mode switches

boolean nomonsters;     // working -nomonsters
boolean respawnparm;    // working -respawn
boolean fastparm;       // working -fast

boolean singletics = false; // debug flag to cancel adaptiveness

//jff 1/22/98 parms for disabling music and sound
boolean nosfxparm;
boolean nomusicparm;

//jff 4/18/98
extern boolean inhelpscreens;

skill_t startskill;
int     startepisode;
int     startmap;
boolean autostart;
FILE    *debugfile;
int ffmap;

boolean advancedemo;

char    wadfile[PATH_MAX+1];       // primary wad file
char    mapdir[PATH_MAX+1];        // directory of development maps
char    baseiwad[PATH_MAX+1];      // jff 3/23/98: iwad directory
char    basesavegame[PATH_MAX+1];  // killough 2/16/98: savegame directory

//jff 4/19/98 list of standard IWAD names
const char *const standard_iwads[]=
{
  "doom2f.wad",
  "doom2.wad",
  "plutonia.wad",
  "tnt.wad",
  "doom.wad",
  "doom1.wad",
  "doomu.wad", /* CPhipps - alow doomu.wad */
  "freedoom.wad", /* wart@kobold.org:  added freedoom for Fedora Extras */
};
//static const int nstandard_iwads = sizeof standard_iwads/sizeof*standard_iwads;

/*
 * D_PostEvent - Event handling
 *
 * Called by I/O functions when an event is received.
 * Try event handlers for each code area in turn.
 * cph - in the true spirit of the Boom source, let the 
 *  short ciruit operator madness begin!
 */

void D_PostEvent(event_t *ev)
{
  /* cph - suppress all input events at game start
   * FIXME: This is a lousy kludge */
  if (gametic < 3) return;
  M_Responder(ev) ||
	  (gamestate == GS_LEVEL && (
				     HU_Responder(ev) ||
				     ST_Responder(ev) ||
				     AM_Responder(ev)
				     )
	  ) ||
	G_Responder(ev);
}

//
// D_Wipe
//
// CPhipps - moved the screen wipe code from D_Display to here
// The screens to wipe between are already stored, this just does the timing
// and screen updating

static void D_Wipe(void)
{
  boolean done;
  int wipestart = I_GetTime () - 1;

  do
    {
      int nowtime, tics;
      do
        {
          I_uSleep(5000); // CPhipps - don't thrash cpu in this loop
          nowtime = I_GetTime();
          tics = nowtime - wipestart;
        }
      while (!tics);
      wipestart = nowtime;
      done = wipe_ScreenWipe(tics);
      I_UpdateNoBlit();
      M_Drawer();                   // menu is drawn even on top of wipes
      I_FinishUpdate();             // page flip or blit buffer
    }
  while (!done);
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t    wipegamestate = GS_DEMOSCREEN;
extern boolean setsizeneeded;
extern int     showMessages;

void D_Display (void)
{
  static boolean inhelpscreensstate   = false;
  static boolean isborderstate        = false;
  static boolean borderwillneedredraw = false;
  static gamestate_t oldgamestate = -1;
  boolean wipe;
  boolean viewactive = false, isborder = false;

  if (nodrawers)                    // for comparative timing / profiling
    return;

  if (!I_StartDisplay())
    return;

  // save the current screen if about to wipe
  if ((wipe = gamestate != wipegamestate) && (V_GetMode() != VID_MODEGL))
    wipe_StartScreen();

  if (gamestate != GS_LEVEL) { // Not a level
    switch ((int)oldgamestate) {
    case -1:
    case GS_LEVEL:
      V_SetPalette(0); // cph - use default (basic) palette
    default:
      break;
    }

    switch (gamestate) {
    case GS_INTERMISSION:
      WI_Drawer();
      break;
    case GS_FINALE:
      F_Drawer();
      break;
    case GS_DEMOSCREEN:
      D_PageDrawer();
      break;
    default:
      break;
    }
  } else if (gametic != basetic) { // In a level
    boolean redrawborderstuff;

    HU_Erase();

    if (setsizeneeded) {               // change the view size if needed
      R_ExecuteSetViewSize();
      oldgamestate = -1;            // force background redraw
    }

    // Work out if the player view is visible, and if there is a border
    viewactive = (!(automapmode & am_active) || (automapmode & am_overlay)) && !inhelpscreens;
    isborder = viewactive ? (viewheight != SCREENHEIGHT) : (!inhelpscreens && (automapmode & am_active));

    if (oldgamestate != GS_LEVEL) {
      R_FillBackScreen ();    // draw the pattern into the back screen
      redrawborderstuff = isborder;
    } else {
      // CPhipps -
      // If there is a border, and either there was no border last time,
      // or the border might need refreshing, then redraw it.
      redrawborderstuff = isborder && (!isborderstate || borderwillneedredraw);
      // The border may need redrawing next time if the border surrounds the screen,
      // and there is a menu being displayed
      borderwillneedredraw = menuactive && isborder && viewactive && (viewwidth != SCREENWIDTH);
    }
    if (redrawborderstuff || (V_GetMode() == VID_MODEGL))
      R_DrawViewBorder();

    // Now do the drawing
	  if (viewactive) {
		  if ( testNewRenderer ) {	// JDC
			  IR_RenderPlayerView (&players[displayplayer]);
 		  } else {
			  R_RenderPlayerView (&players[displayplayer]);
		  }
	  }
	  if (automapmode & am_active) {
      AM_Drawer();
	  }
    ST_Drawer((viewheight != SCREENHEIGHT) || ((automapmode & am_active) && !(automapmode & am_overlay)), redrawborderstuff);
    if (V_GetMode() != VID_MODEGL)
      R_DrawViewBorder();
    HU_Drawer();
  }

  inhelpscreensstate = inhelpscreens;
  isborderstate      = isborder;
  oldgamestate = wipegamestate = gamestate;

  // draw pause pic
  if (paused) {
    // Simplified the "logic" here and no need for x-coord caching - POPE
    V_DrawNamePatch((320 - V_NamePatchWidth("M_PAUSE"))/2, 4,
                    0, "M_PAUSE", CR_DEFAULT, VPT_STRETCH);
  }

  // menus go directly to the screen
  M_Drawer();          // menu is drawn even on top of everything
#ifdef HAVE_NET
  NetUpdate();         // send out any new accumulation
#else
  D_BuildNewTiccmds();
#endif

  // normal update
  if (!wipe || (V_GetMode() == VID_MODEGL))
    I_FinishUpdate ();              // page flip or blit buffer
  else {
    // wipe update
    wipe_EndScreen();
    D_Wipe();
  }

  I_EndDisplay();

  //e6y: don't thrash cpu during pausing
  if (paused) {
    I_uSleep(1000);
  }
}

// CPhipps - Auto screenshot Variables

static int auto_shot_count, auto_shot_time;
static const char *auto_shot_fname;

//
//  D_DoomLoop()
//
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//

// This function is unused on iOS.
#ifndef IPHONE
static void D_DoomLoop(void)
{
  for (;;)
    {
      WasRenderedInTryRunTics = false;
      // frame syncronous IO operations
      I_StartFrame ();

      if (ffmap == gamemap) ffmap = 0;

      // process one or more tics
      if (singletics)
        {
          I_StartTic ();
          G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
          if (advancedemo)
            D_DoAdvanceDemo ();
          M_Ticker ();
          G_Ticker ();
          P_Checksum(gametic);
          gametic++;
          maketic++;
        }
      else
        TryRunTics (); // will run at least one tic

      // killough 3/16/98: change consoleplayer to displayplayer
      if (players[displayplayer].mo) // cph 2002/08/10
	S_UpdateSounds(players[displayplayer].mo);// move positional sounds

      if (V_GetMode() == VID_MODEGL ? 
        !movement_smooth || !WasRenderedInTryRunTics :
        !movement_smooth || !WasRenderedInTryRunTics || gamestate != wipegamestate
      )
        {
        // Update display, next frame, with current state.
        D_Display();
      }

      // CPhipps - auto screenshot
      if (auto_shot_fname && !--auto_shot_count) {
  auto_shot_count = auto_shot_time;
  M_DoScreenShot(auto_shot_fname);
      }
    }
}
#endif

//
//  DEMO LOOP
//

static int  demosequence;         // killough 5/2/98: made static
static int  pagetic;
static const char *pagename; // CPhipps - const

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
  if (--pagetic < 0)
    D_AdvanceDemo();
}

//
// D_PageDrawer
//
static void D_PageDrawer(void)
{
  // proff/nicolas 09/14/98 -- now stretchs bitmaps to fullscreen!
  // CPhipps - updated for new patch drawing
  // proff - added M_DrawCredits
  if (pagename)
  {
    V_DrawNamePatch(0, 0, 0, pagename, CR_DEFAULT, VPT_STRETCH);
  }
  else
    M_DrawCredits();
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
  advancedemo = true;
}

/* killough 11/98: functions to perform demo sequences
 * cphipps 10/99: constness fixes
 */

static void D_SetPageName(const char *name)
{
  pagename = name;
}

static void D_DrawTitle1(const char *name)
{
  S_StartMusic(mus_intro);
  pagetic = (TICRATE*170)/35;
  D_SetPageName(name);
}

static void D_DrawTitle2(const char *name)
{
  S_StartMusic(mus_dm2ttl);
  D_SetPageName(name);
}

/* killough 11/98: tabulate demo sequences
 */

static struct
{
  void (*func)(const char *);
  const char *name;
} const demostates[][4] =
  {
    {
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
      {D_DrawTitle2, "TITLEPIC"},
      {D_DrawTitle1, "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
      {G_DeferedPlayDemo, "demo1"},
    },
    {
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
      {D_SetPageName, NULL},
    },

    {
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
      {G_DeferedPlayDemo, "demo2"},
    },

    {
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "HELP2"},
      {D_SetPageName, "CREDIT"},
      {D_DrawTitle1,  "TITLEPIC"},
    },

    {
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
      {G_DeferedPlayDemo, "demo3"},
    },

    {
      {NULL, NULL},
      {NULL, NULL},
      {NULL, NULL},
      {D_SetPageName, "CREDIT"},
    },

    {
      {NULL, NULL},
      {NULL, NULL},
      {NULL, NULL},
      {G_DeferedPlayDemo, "demo4"},
    },

    {
      {NULL, NULL},
      {NULL, NULL},
      {NULL, NULL},
      {NULL, NULL},
    }
  };

/*
 * This cycles through the demo sequences.
 * killough 11/98: made table-driven
 */

void D_DoAdvanceDemo(void)
{
  players[consoleplayer].playerstate = PST_LIVE;  /* not reborn */
  advancedemo = usergame = paused = false;
  gameaction = ga_nothing;

  pagetic = TICRATE * 11;         /* killough 11/98: default behavior */
  gamestate = GS_DEMOSCREEN;

  if (netgame && !demoplayback) {
    demosequence = 0;
  } else
   if (!demostates[++demosequence][gamemode].func)
    demosequence = 0;
  demostates[demosequence][gamemode].func
    (demostates[demosequence][gamemode].name);
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo();
}

//
// D_AddFile
//
// Rewritten by Lee Killough
//
// Ty 08/29/98 - add source parm to indicate where this came from
// CPhipps - static, const char* parameter
//         - source is an enum
//         - modified to allocate & use new wadfiles array
void D_AddFile (const char *file, wad_source_t source)
{
  char *gwa_filename=NULL;

  wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
  wadfiles[numwadfiles].name =
    AddDefaultExtension(strcpy(malloc(strlen(file)+5), file), ".wad");
  wadfiles[numwadfiles].src = source; // Ty 08/29/98
  numwadfiles++;
  // proff: automatically try to add the gwa files
  // proff - moved from w_wad.c
  gwa_filename=AddDefaultExtension(strcpy(malloc(strlen(file)+5), file), ".wad");
  if (strlen(gwa_filename)>4)
    if (!strcasecmp(gwa_filename+(strlen(gwa_filename)-4),".wad"))
    {
      char *ext;
      ext = &gwa_filename[strlen(gwa_filename)-4];
      ext[1] = 'g'; ext[2] = 'w'; ext[3] = 'a';
      wadfiles = realloc(wadfiles, sizeof(*wadfiles)*(numwadfiles+1));
      wadfiles[numwadfiles].name = gwa_filename;
      wadfiles[numwadfiles].src = source; // Ty 08/29/98
      numwadfiles++;
    }
}

// killough 10/98: support -dehout filename
// cph - made const, don't cache results
static const char *D_dehout(void)
{
  int p = M_CheckParm("-dehout");
  if (!p)
    p = M_CheckParm("-bexout");
  return (p && ++p < myargc ? myargv[p] : NULL);
}

//
// CheckIWAD
//
// Verify a file is indeed tagged as an IWAD
// Scan its lumps for levelnames and return gamemode as indicated
// Detect missing wolf levels in DOOM II
//
// The filename to check is passed in iwadname, the gamemode detected is
// returned in gmode, hassec returns the presence of secret levels
//
// jff 4/19/98 Add routine to test IWAD for validity and determine
// the gamemode from it. Also note if DOOM II, whether secret levels exist
// CPhipps - const char* for iwadname, made static
static void CheckIWAD(const char *iwadname,GameMode_t *gmode,boolean *hassec)
{
  if ( !access (iwadname,R_OK) )
  {
    int ud=0,rg=0,sw=0,cm=0,sc=0;
    FILE* fp;

    // Identify IWAD correctly
    if ((fp = fopen(iwadname, "rb")))
    {
      wadinfo_t header;

      // read IWAD header
      if (fread(&header, sizeof(header), 1, fp) == 1 && !strncmp(header.identification, "IWAD", 4))
      {
        size_t length;
        filelump_t *fileinfo;

        // read IWAD directory
        header.numlumps = (int)LONG(header.numlumps);
        header.infotableofs = (int)LONG(header.infotableofs);
        length = header.numlumps;
        fileinfo = malloc(length*sizeof(filelump_t));
        if (fseek (fp, header.infotableofs, SEEK_SET) ||
            fread (fileinfo, sizeof(filelump_t), length, fp) != length ||
            fclose(fp))
          I_Error("CheckIWAD: failed to read directory %s",iwadname);

        // scan directory for levelname lumps
        while (length--)
          if (fileinfo[length].name[0] == 'E' &&
              fileinfo[length].name[2] == 'M' &&
              fileinfo[length].name[4] == 0)
          {
            if (fileinfo[length].name[1] == '4')
              ++ud;
            else if (fileinfo[length].name[1] == '3')
              ++rg;
            else if (fileinfo[length].name[1] == '2')
              ++rg;
            else if (fileinfo[length].name[1] == '1')
              ++sw;
          }
          else if (fileinfo[length].name[0] == 'M' &&
                    fileinfo[length].name[1] == 'A' &&
                    fileinfo[length].name[2] == 'P' &&
                    fileinfo[length].name[5] == 0)
          {
            ++cm;
            if (fileinfo[length].name[3] == '3')
              if (fileinfo[length].name[4] == '1' ||
                  fileinfo[length].name[4] == '2')
                ++sc;
          }

        free(fileinfo);
      }
      else // missing IWAD tag in header
        I_Error("CheckIWAD: IWAD tag %s not present", iwadname);
    }
    else // error from open call
      I_Error("CheckIWAD: Can't open IWAD %s", iwadname);

    // Determine game mode from levels present
    // Must be a full set for whichever mode is present
    // Lack of wolf-3d levels also detected here

    *gmode = indetermined;
    *hassec = false;
    if (cm>=30)
    {
      *gmode = commercial;
      *hassec = sc>=2;
    }
    else if (ud>=9)
      *gmode = retail;
    else if (rg>=18)
      *gmode = registered;
    else if (sw>=9)
      *gmode = shareware;
  }
  else // error from access call
    I_Error("CheckIWAD: IWAD %s not readable", iwadname);
}



// NormalizeSlashes
//
// Remove trailing slashes, translate backslashes to slashes
// The string to normalize is passed and returned in str
//
// jff 4/19/98 Make killoughs slash fixer a subroutine
//
static void NormalizeSlashes(char *str)
{
  int l;

  // killough 1/18/98: Neater / \ handling.
  // Remove trailing / or \ to prevent // /\ \/ \\, and change \ to /

  if (!str || !(l = (int)strlen(str)))
    return;
  if (str[--l]=='/' || str[l]=='\\')     // killough 1/18/98
    str[l]=0;
  while (l--)
    if (str[l]=='\\')
      str[l]='/';
}

/*
 * FindIWADFIle
 *
 * Search for one of the standard IWADs
 * CPhipps  - static, proper prototype
 *    - 12/1999 - rewritten to use I_FindFile
 */
// TODO: We may restore this function later, if we send in iwad selection as a "parameter"
// in order to remain closer to vanilla prboom.
#ifndef IPHONE
static char *FindIWADFile(void)
{
  int   i;
  char  * iwad  = NULL;

  i = M_CheckParm("-iwad");
  if (i && (++i < myargc)) {
    iwad = I_FindFile(myargv[i], ".wad");
  } else {
    for (i=0; !iwad && i<nstandard_iwads; i++)
      iwad = I_FindFile(standard_iwads[i], ".wad");
  }
  return iwad;
}
#endif

//
// IdentifyVersion
//
// Set the location of the defaults file and the savegame root
// Locate and validate an IWAD file
// Determine gamemode from the IWAD
//
// supports IWADs with custom names. Also allows the -iwad parameter to
// specify which iwad is being searched for if several exist in one dir.
// The -iwad parm may specify:
//
// 1) a specific pathname, which must exist (.wad optional)
// 2) or a directory, which must contain a standard IWAD,
// 3) or a filename, which must be found in one of the standard places:
//   a) current dir,
//   b) exe dir
//   c) $DOOMWADDIR
//   d) or $HOME
//
// jff 4/19/98 rewritten to use a more advanced search algorithm
// 
// Added an iwad parameter to IdentifyVersion - this makes it easier to use this
// code in a library shared between multiple games.

static void IdentifyVersion ( const char * iwad )
{
  int         i;    //jff 3/24/98 index of args on commandline
  struct stat sbuf; //jff 3/24/98 used to test save path for existence
  //const char *iwad;

  // set save path to -save parm or current dir

  //jff 3/27/98 default to current dir
  //V.Aguilar (5/30/99): In LiNUX, default to $HOME/.lxdoom
  {
    // CPhipps - use DOOMSAVEDIR if defined
    char* p = getenv("DOOMSAVEDIR");

    if (p != NULL)
      if (strlen(p) > PATH_MAX-12) p = NULL;

    strcpy(basesavegame,(p == NULL) ? I_DoomExeDir() : p);
  }
  if ((i=M_CheckParm("-save")) && i<myargc-1) //jff 3/24/98 if -save present
  {
    if (!stat(myargv[i+1],&sbuf) && S_ISDIR(sbuf.st_mode)) // and is a dir
    {
      strcpy(basesavegame,myargv[i+1]);  //jff 3/24/98 use that for savegame
      NormalizeSlashes(basesavegame);    //jff 9/22/98 fix c:\ not working
    }
    //jff 9/3/98 use logical output routine
    else lprintf(LO_ERROR,"Error: -save path does not exist, using %s\n", basesavegame);
  }

  // locate the IWAD and determine game mode from it

  //iwad = FindIWADFile();
  //iwad = iphoneFindIWADFile();
	
#if (defined(GL_DOOM) && defined(_DEBUG))
  // proff 11/99: used for debugging
  {
    FILE *f;
    f=fopen("levelinfo.txt","w");
    if (f)
    {
      fprintf(f,"%s\n",iwad);
      fclose(f);
    }
  }
#endif

  if (iwad && *iwad)
  {
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"IWAD found: %s\n",iwad); //jff 4/20/98 print only if found
    CheckIWAD(iwad,&gamemode,&haswolflevels);

    /* jff 8/23/98 set gamemission global appropriately in all cases
     * cphipps 12/1999 - no version output here, leave that to the caller
     */
    switch(gamemode)
    {
      case retail:
      case registered:
      case shareware:
        gamemission = doom;
        break;
      case commercial:
        i = (int)strlen(iwad);
        gamemission = doom2;
        if (i>=10 && !strnicmp(iwad+i-10,"doom2f.wad",10))
          language=french;
        else if (i>=7 && !strnicmp(iwad+i-7,"tnt.wad",7))
          gamemission = pack_tnt;
        else if (i>=12 && !strnicmp(iwad+i-12,"plutonia.wad",12))
          gamemission = pack_plut;
        break;
      default:
        gamemission = none;
        break;
    }
    if (gamemode == indetermined)
      //jff 9/3/98 use logical output routine
      lprintf(LO_WARN,"Unknown Game Version, may not work\n");
    D_AddFile(iwad,source_iwad);
	
    //free(iwad);
  }
  else
    I_Error("IdentifyVersion: IWAD not found\n");
}



// killough 5/3/98: old code removed
//
// Find a Response File
//

#define MAXARGVS 100

static void FindResponseFile (void)
{
  int i;

  for (i = 1;i < myargc;i++)
    if (myargv[i][0] == '@')
      {
        int  size;
        int  index;
	int indexinfile;
        byte *file = NULL;
        const char **moreargs = malloc(myargc * sizeof(const char*));
        const char **newargv;
        // proff 04/05/2000: Added for searching responsefile
        char fname[PATH_MAX+1];

        strcpy(fname,&myargv[i][1]);
        AddDefaultExtension(fname,".rsp");

        // READ THE RESPONSE FILE INTO MEMORY
        // proff 04/05/2000: changed for searching responsefile
        // cph 2002/08/09 - use M_ReadFile for simplicity
	size = M_ReadFile(fname, &file);
        // proff 04/05/2000: Added for searching responsefile
        if (size < 0)
        {
          strcat(strcpy(fname,I_DoomExeDir()),&myargv[i][1]);
          AddDefaultExtension(fname,".rsp");
	  size = M_ReadFile(fname, &file);
        }
        if (size < 0)
        {
            /* proff 04/05/2000: Changed from LO_FATAL
             * proff 04/05/2000: Simply removed the exit(1);
       * cph - made fatal, don't drop through and SEGV
       */
            I_Error("No such response file: %s",fname);
        }
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"Found response file %s\n",fname);
        // proff 04/05/2000: Added check for empty rsp file
        if (size<=0)
        {
	  int k;
          lprintf(LO_ERROR,"\nResponse file empty!\n");

	  newargv = calloc(sizeof(char *),MAXARGVS);
	  newargv[0] = myargv[0];
          for (k = 1,index = 1;k < myargc;k++)
          {
            if (i!=k)
              newargv[index++] = myargv[k];
          }
          myargc = index; myargv = newargv;
          return;
        }

        // KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
	memcpy((void *)moreargs,&myargv[i+1],(index = myargc - i - 1) * sizeof(myargv[0]));

	{
	  const char *firstargv = myargv[0];
	  newargv = calloc(sizeof(char *),MAXARGVS);
	  newargv[0] = firstargv;
	}

        {
	  byte *infile = file;
	  indexinfile = 0;
	  indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
	  do {
	    while (size > 0 && isspace(*infile)) { infile++; size--; }
	    if (size > 0) {
	      char *s = malloc(size+1);
	      char *p = s;
	      int quoted = 0; 

	      while (size > 0) {
		// Whitespace terminates the token unless quoted
		if (!quoted && isspace(*infile)) break;
		if (*infile == '\"') {
		  // Quotes are removed but remembered
		  infile++; size--; quoted ^= 1; 
		} else {
		  *p++ = *infile++; size--;
		}
	      }
	      if (quoted) I_Error("Runaway quoted string in response file");

	      // Terminate string, realloc and add to argv
	      *p = 0;
	      newargv[indexinfile++] = realloc(s,strlen(s)+1);
	    }
	  } while(size > 0);
	}
	free(file);

	memcpy((void *)&newargv[indexinfile],moreargs,index*sizeof(moreargs[0]));
	free((void *)moreargs);

        myargc = indexinfile+index; myargv = newargv;

        // DISPLAY ARGS
        //jff 9/3/98 use logical output routine
        lprintf(LO_CONFIRM,"%d command-line args:\n",myargc);
	for (index=1;index<myargc;index++)
	  //jff 9/3/98 use logical output routine
          lprintf(LO_CONFIRM,"%s\n",myargv[index]);
        break;
      }
}

//
// DoLooseFiles
//
// Take any file names on the command line before the first switch parm
// and insert the appropriate -file, -deh or -playdemo switch in front
// of them.
//
// Note that more than one -file, etc. entry on the command line won't
// work, so we have to go get all the valid ones if any that show up
// after the loose ones.  This means that boom fred.wad -file wilma
// will still load fred.wad and wilma.wad, in that order.
// The response file code kludges up its own version of myargv[] and
// unfortunately we have to do the same here because that kludge only
// happens if there _is_ a response file.  Truth is, it's more likely
// that there will be a need to do one or the other so it probably
// isn't important.  We'll point off to the original argv[], or the
// area allocated in FindResponseFile, or our own areas from strdups.
//
// CPhipps - OUCH! Writing into *myargv is too dodgy, damn

static void DoLooseFiles(void)
{
  char *wads[MAXARGVS];  // store the respective loose filenames
  char *lmps[MAXARGVS];
  char *dehs[MAXARGVS];
  int wadcount = 0;      // count the loose filenames
  int lmpcount = 0;
  int dehcount = 0;
  int i,j,p;
  const char **tmyargv;  // use these to recreate the argv array
  int tmyargc;
  boolean skip[MAXARGVS]; // CPhipps - should these be skipped at the end

  for (i=0; i<MAXARGVS; i++)
    skip[i] = false;

  for (i=1;i<myargc;i++)
  {
    if (*myargv[i] == '-') break;  // quit at first switch

    // so now we must have a loose file.  Find out what kind and store it.
    j = (int)strlen(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".wad"))
      wads[wadcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".lmp"))
      lmps[lmpcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".deh"))
      dehs[dehcount++] = strdup(myargv[i]);
    if (!stricmp(&myargv[i][j-4],".bex"))
      dehs[dehcount++] = strdup(myargv[i]);
    if (myargv[i][j-4] != '.')  // assume wad if no extension
      wads[wadcount++] = strdup(myargv[i]);
    skip[i] = true; // nuke that entry so it won't repeat later
  }

  // Now, if we didn't find any loose files, we can just leave.
  if (wadcount+lmpcount+dehcount == 0) return;  // ******* early return ****

  if ((p = M_CheckParm ("-file")))
  {
    skip[p] = true;    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      wads[wadcount++] = strdup(myargv[p]);
      skip[p] = true;  // null any we find and save
    }
  }

  if ((p = M_CheckParm ("-deh")))
  {
    skip[p] = true;    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      dehs[dehcount++] = strdup(myargv[p]);
      skip[p] = true;  // null any we find and save
    }
  }

  if ((p = M_CheckParm ("-playdemo")))
  {
    skip[p] = true;    // nuke the entry
    while (++p != myargc && *myargv[p] != '-')
    {
      lmps[lmpcount++] = strdup(myargv[p]);
      skip[p] = true;  // null any we find and save
    }
  }

  // Now go back and redo the whole myargv array with our stuff in it.
  // First, create a new myargv array to copy into
  tmyargv = calloc(sizeof(char *),MAXARGVS);
  tmyargv[0] = myargv[0]; // invocation
  tmyargc = 1;

  // put our stuff into it
  if (wadcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-file"); // put the switch in
    for (i=0;i<wadcount;)
      tmyargv[tmyargc++] = wads[i++]; // allocated by strdup above
  }

  // for -deh
  if (dehcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-deh");
    for (i=0;i<dehcount;)
      tmyargv[tmyargc++] = dehs[i++];
  }

  // for -playdemo
  if (lmpcount > 0)
  {
    tmyargv[tmyargc++] = strdup("-playdemo");
    for (i=0;i<lmpcount;)
      tmyargv[tmyargc++] = lmps[i++];
  }

  // then copy everything that's there now
  for (i=1;i<myargc;i++)
  {
    if (!skip[i])  // skip any zapped entries
      tmyargv[tmyargc++] = myargv[i];  // pointers are still valid
  }
  // now make the global variables point to our array
  myargv = tmyargv;
  myargc = tmyargc;
}

/* cph - MBF-like wad/deh/bex autoload code */
const char *wad_files[MAXLOADFILES], *deh_files[MAXLOADFILES];

// CPhipps - misc screen stuff
int desired_screenwidth, desired_screenheight;

static void L_SetupConsoleMasks(void) {
  int p;
  int i;
  const char *cena="ICWEFDA",*pos;  //jff 9/3/98 use this for parsing console masks // CPhipps - const char*'s

  //jff 9/3/98 get mask for console output filter
  if ((p = M_CheckParm ("-cout"))) {
    lprintf(LO_DEBUG, "mask for stdout console output: ");
    if (++p != myargc && *myargv[p] != '-')
      for (i=0,cons_output_mask=0;(size_t)i<strlen(myargv[p]);i++)
        if ((pos = strchr(cena,toupper(myargv[p][i])))) {
          cons_output_mask |= (1<<(pos-cena));
          lprintf(LO_DEBUG, "%c", toupper(myargv[p][i]));
        }
    lprintf(LO_DEBUG, "\n");
  }

  //jff 9/3/98 get mask for redirected console error filter
  if ((p = M_CheckParm ("-cerr"))) {
    lprintf(LO_DEBUG, "mask for stderr console output: ");
    if (++p != myargc && *myargv[p] != '-')
      for (i=0,cons_error_mask=0;(size_t)i<strlen(myargv[p]);i++)
        if ((pos = strchr(cena,toupper(myargv[p][i])))) {
          cons_error_mask |= (1<<(pos-cena));
          lprintf(LO_DEBUG, "%c", toupper(myargv[p][i]));
        }
    lprintf(LO_DEBUG, "\n");
  }
}

//
// D_DoomMainSetup
//
// CPhipps - the old contents of D_DoomMain, but moved out of the main
//  line of execution so its stack space can be freed
//
// Added iwad and pwad parameters to facilitate using this code in a library shared between
// multiple games.
void iphoneAddPWADFiles(void);

/* JDC static */ void D_DoomMainSetup( const char * iwad, const char * pwad )
{
  int p,slot;
  
    numwadfiles = 0;
    R_FlushAllPatches();
    gld_CleanMemory();
    
    
  L_SetupConsoleMasks();

  setbuf(stdout,NULL);

  // proff 04/05/2000: Added support for include response files
  /* proff 2001/7/1 - Moved up, so -config can be in response files */
  {
    boolean rsp_found;
    int i;

    do {
      rsp_found=false;
      for (i=0; i<myargc; i++)
        if (myargv[i][0]=='@')
          rsp_found=true;
      FindResponseFile();
    } while (rsp_found==true);
  }

  lprintf(LO_INFO,"M_LoadDefaults: Load system defaults.\n");
  M_LoadDefaults();              // load before initing other systems

  // figgi 09/18/00-- added switch to force classic bsp nodes
  if (M_CheckParm ("-forceoldbsp"))
  {
    extern boolean forceOldBsp;
    forceOldBsp = true;
  }

  D_BuildBEXTables(); // haleyjd

  DoLooseFiles();  // Ty 08/29/98 - handle "loose" files on command line
  IdentifyVersion( iwad );

  // Load prboom.wad after IWAD but before everything else
  {
    char data_wad_path[ 1024 ];
    I_FindFile(PACKAGE ".wad", ".wad", data_wad_path );

    D_AddFile(data_wad_path, source_pre);
  }
	
  // e6y: DEH files preloaded in wrong order
  // http://sourceforge.net/tracker/index.php?func=detail&aid=1418158&group_id=148658&atid=772943
  // The dachaked stuff has been moved below an autoload

  // jff 1/24/98 set both working and command line value of play parms
  nomonsters = clnomonsters = M_CheckParm ("-nomonsters");
  respawnparm = clrespawnparm = M_CheckParm ("-respawn");
  fastparm = clfastparm = M_CheckParm ("-fast");
  // jff 1/24/98 end of set to both working and command line value

  devparm = M_CheckParm ("-devparm");

  if (M_CheckParm ("-altdeath"))
    deathmatch = 2;
  else
    if (M_CheckParm ("-deathmatch"))
      deathmatch = 1;

  {
    // CPhipps - localise title variable
    // print title for every printed line
    // cph - code cleaned and made smaller
    const char* doomverstr;

    switch ( gamemode ) {
    case retail:
      doomverstr = "The Ultimate DOOM";
      break;
    case shareware:
      doomverstr = "DOOM Shareware";
      break;
    case registered:
      doomverstr = "DOOM Registered";
      break;
    case commercial:  // Ty 08/27/98 - fixed gamemode vs gamemission
      switch (gamemission)
      {
        case pack_plut:
    doomverstr = "DOOM 2: Plutonia Experiment";
          break;
        case pack_tnt:
          doomverstr = "DOOM 2: TNT - Evilution";
          break;
        default:
          doomverstr = "DOOM 2: Hell on Earth";
          break;
      }
      break;
    default:
      doomverstr = "Public DOOM";
      break;
    }

    /* cphipps - the main display. This shows the build date, copyright, and game type */
    lprintf(LO_ALWAYS,"PrBoom (built %s), playing: %s\n"
      "PrBoom is released under the GNU General Public license v2.0.\n"
      "You are welcome to redistribute it under certain conditions.\n"
      "It comes with ABSOLUTELY NO WARRANTY. See the file COPYING for details.\n",
      version_date, doomverstr);
  }

  if (devparm)
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"%s",D_DEVSTR);

  // turbo option
  if ((p=M_CheckParm ("-turbo")))
    {
      int scale = 200;
      extern int forwardmove[2];
      extern int sidemove[2];

      if (p<myargc-1)
        scale = atoi(myargv[p+1]);
      if (scale < 10)
        scale = 10;
      if (scale > 400)
        scale = 400;
      //jff 9/3/98 use logical output routine
      lprintf (LO_CONFIRM,"turbo scale: %i%%\n",scale);
      forwardmove[0] = forwardmove[0]*scale/100;
      forwardmove[1] = forwardmove[1]*scale/100;
      sidemove[0] = sidemove[0]*scale/100;
      sidemove[1] = sidemove[1]*scale/100;
    }

  modifiedgame = false;

  // get skill / episode / map from parms

  startskill = sk_none; // jff 3/24/98 was sk_medium, just note not picked
  startepisode = 1;
  startmap = 1;
  autostart = false;

  if ((p = M_CheckParm ("-skill")) && p < myargc-1)
    {
      startskill = myargv[p+1][0]-'1';
      autostart = true;
    }

  if ((p = M_CheckParm ("-episode")) && p < myargc-1)
    {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
    }

  if ((p = M_CheckParm ("-timer")) && p < myargc-1 && deathmatch)
    {
      int time = atoi(myargv[p+1]);
      //jff 9/3/98 use logical output routine
      lprintf(LO_CONFIRM,"Levels will end after %d minute%s.\n", time, time>1 ? "s" : "");
    }

  if ((p = M_CheckParm ("-avg")) && p < myargc-1 && deathmatch)
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"Austin Virtual Gaming: Levels will end after 20 minutes\n");

  if ((p = M_CheckParm ("-warp")) ||      // killough 5/2/98
       (p = M_CheckParm ("-wart")))
       // Ty 08/29/98 - moved this check later so we can have -warp alone: && p < myargc-1)
  {
    startmap = 0; // Ty 08/29/98 - allow "-warp x" to go to first map in wad(s)
    autostart = true; // Ty 08/29/98 - move outside the decision tree
    if (gamemode == commercial)
    {
      if (p < myargc-1)
        startmap = atoi(myargv[p+1]);   // Ty 08/29/98 - add test if last parm
    }
    else    // 1/25/98 killough: fix -warp xxx from crashing Doom 1 / UD
    {
      if (p < myargc-2)
      {
        startepisode = atoi(myargv[++p]);
        startmap = atoi(myargv[p+1]);
      }
    }
  }
  // Ty 08/29/98 - later we'll check for startmap=0 and autostart=true
  // as a special case that -warp * was used.  Actually -warp with any
  // non-numeric will do that but we'll only document "*"

  //jff 1/22/98 add command line parms to disable sound and music
  {
    int nosound = M_CheckParm("-nosound");
    nomusicparm = nosound || M_CheckParm("-nomusic");
    nosfxparm   = nosound || M_CheckParm("-nosfx");
  }
  //jff end of sound/music command line parms

  // killough 3/2/98: allow -nodraw -noblit generally
  nodrawers = M_CheckParm ("-nodraw");
  noblit = M_CheckParm ("-noblit");

  //proff 11/22/98: Added setting of viewangleoffset
  p = M_CheckParm("-viewangle");
  if (p)
  {
    viewangleoffset = atoi(myargv[p+1]);
    viewangleoffset = viewangleoffset<0 ? 0 : (viewangleoffset>7 ? 7 : viewangleoffset);
    viewangleoffset = (8-viewangleoffset) * ANG45;
  }

  // init subsystems

  G_ReloadDefaults();    // killough 3/4/98: set defaults just loaded.
  // jff 3/24/98 this sets startskill if it was -1

  // Video stuff
  if ((p = M_CheckParm("-width")))
    if (myargv[p+1])
      desired_screenwidth = atoi(myargv[p+1]);

  if ((p = M_CheckParm("-height")))
    if (myargv[p+1])
      desired_screenheight = atoi(myargv[p+1]);

  if ((p = M_CheckParm("-fullscreen")))
      use_fullscreen = 1;

  if ((p = M_CheckParm("-nofullscreen")))
      use_fullscreen = 0;

  // e6y
  // New command-line options for setting a window (-window) 
  // or fullscreen (-nowindow) mode temporarily which is not saved in cfg.
  // It works like "-geom" switch
  desired_fullscreen = use_fullscreen;
  if ((p = M_CheckParm("-window")))
      desired_fullscreen = 0;

  if ((p = M_CheckParm("-nowindow")))
      desired_fullscreen = 1;

  { // -geometry handling, change screen size for this session only
    // e6y: new code by me
    int w, h;

    if (!(p = M_CheckParm("-geom")))
      p = M_CheckParm("-geometry");

    if (!(p && (p+1<myargc) && sscanf(myargv[p+1], "%dx%d", &w, &h) == 2))
    {
      w = desired_screenwidth;
      h = desired_screenheight;
    }
    I_CalculateRes(w, h);
  }

#ifdef GL_DOOM
  // proff 04/05/2000: for GL-specific switches
  gld_InitCommandLine();
#endif

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"V_Init: allocate screens.\n");
  V_Init();

  // CPhipps - autoloading of wads
  // Designed to be general, instead of specific to boomlump.wad
  // Some people might find this useful
  // cph - support MBF -noload parameter
  if (!M_CheckParm("-noload")) {
    int i;

    for (i=0; i<MAXLOADFILES*2; i++) {
      const char *fname = (i < MAXLOADFILES) ? wad_files[i]
  : deh_files[i - MAXLOADFILES];
      char fpath[ 1024 ];

      if (!(fname && *fname)) continue;
      // Filename is now stored as a zero terminated string
      I_FindFile(fname, (i < MAXLOADFILES) ? ".wad" : ".bex", fpath);

        if (i >= MAXLOADFILES)
          ProcessDehFile(fpath, D_dehout(), 0);
        else {
          D_AddFile(fpath,source_auto_load);
        }
        modifiedgame = true;

    }
  }

  // e6y: DEH files preloaded in wrong order
  // http://sourceforge.net/tracker/index.php?func=detail&aid=1418158&group_id=148658&atid=772943
  // The dachaked stuff has been moved from above

  // ty 03/09/98 do dehacked stuff
  // Note: do this before any other since it is expected by
  // the deh patch author that this is actually part of the EXE itself
  // Using -deh in BOOM, others use -dehacked.
  // Ty 03/18/98 also allow .bex extension.  .bex overrides if both exist.

  p = M_CheckParm ("-deh");
  if (p)
    {
      char file[PATH_MAX+1];      // cph - localised
      // the parms after p are deh/bex file names,
      // until end of parms or another - preceded parm
      // Ty 04/11/98 - Allow multiple -deh files in a row

      while (++p != myargc && *myargv[p] != '-')
        {
          AddDefaultExtension(strcpy(file, myargv[p]), ".bex");
          if (access(file, F_OK))  // nope
            {
              AddDefaultExtension(strcpy(file, myargv[p]), ".deh");
              if (access(file, F_OK))  // still nope
                I_Error("D_DoomMainSetup: Cannot find .deh or .bex file named %s",myargv[p]);
            }
          // during the beta we have debug output to dehout.txt
          ProcessDehFile(file,D_dehout(),0);
        }
    }
  // ty 03/09/98 end of do dehacked stuff

  // add any files specified on the command line with -file wadfile
  // to the wad list

  // killough 1/31/98, 5/2/98: reload hack removed, -wart same as -warp now.

  if ((p = M_CheckParm ("-file")))
    {
      // the parms after p are wadfile/lump names,
      // until end of parms or another - preceded parm
      modifiedgame = true;            // homebrew levels
      while (++p != myargc && *myargv[p] != '-')
        D_AddFile(myargv[p],source_pwad);
    }

	// Add any iphone pwads (for example, No Rest for the Living is a pwad).
    while(pwad && *pwad) {
        char pwad_to_add[1024];
        char* pwadloc_end = strchr(pwad,PWAD_LIST_SEPARATOR);
        unsigned long i = 0;
        if( pwadloc_end ) {
            while( pwad != pwadloc_end ) {
                pwad_to_add[i++] = *pwad++;
            }
            pwad_to_add[i] = '\0';
            pwad++; // advance past separator
            D_AddFile( pwad_to_add, source_pwad );
        } else {
            Com_Printf("Error processing PWADs: %s (%X)", pwad,pwadloc_end);
            break;
        }
	}
    
  if (!(p = M_CheckParm("-playdemo")) || p >= myargc-1) {   /* killough */
    if ((p = M_CheckParm ("-fastdemo")) && p < myargc-1)    /* killough */
      fastdemo = true;             // run at fastest speed possible
    else
      p = M_CheckParm ("-timedemo");
  }

  if (p && p < myargc-1)
    {
      char file[PATH_MAX+1];      // cph - localised
      strcpy(file,myargv[p+1]);
      AddDefaultExtension(file,".lmp");     // killough
      D_AddFile (file,source_lmp);
      //jff 9/3/98 use logical output routine
      lprintf(LO_CONFIRM,"Playing demo %s\n",file);
      if ((p = M_CheckParm ("-ffmap")) && p < myargc-1) {
        ffmap = atoi(myargv[p+1]);
      }

    }

  // internal translucency set to config file value               // phares
  general_translucency = default_translucency;                    // phares

  // 1/18/98 killough: Z_Init() call moved to i_main.c

  // CPhipps - move up netgame init
  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"D_InitNetGame: Checking for network game.\n");
  D_InitNetGame();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"W_Init: Init WADfiles.\n");
  W_Init(); // CPhipps - handling of wadfiles init changed

  lprintf(LO_INFO,"\n");     // killough 3/6/98: add a newline, by popular demand :)

  // e6y 
  // option to disable automatic loading of dehacked-in-wad lump
  if (!M_CheckParm ("-nodeh"))
    if ((p = W_CheckNumForName("DEHACKED")) != -1) // cph - add dehacked-in-a-wad support
      ProcessDehFile(NULL, D_dehout(), p);

  V_InitColorTranslation(); //jff 4/24/98 load color translation lumps

    // JDS: replace sounds with lumps
    I_OverwriteSoundBuffersWithLumps();
    
  // killough 2/22/98: copyright / "modified game" / SPA banners removed

  // Ty 04/08/98 - Add 5 lines of misc. data, only if nonblank
  // The expectation is that these will be set in a .bex file
  //jff 9/3/98 use logical output routine
  if (*startup1) lprintf(LO_INFO,"%s",startup1);
  if (*startup2) lprintf(LO_INFO,"%s",startup2);
  if (*startup3) lprintf(LO_INFO,"%s",startup3);
  if (*startup4) lprintf(LO_INFO,"%s",startup4);
  if (*startup5) lprintf(LO_INFO,"%s",startup5);
  // End new startup strings

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"M_Init: Init miscellaneous info.\n");
  //M_Init();

#ifdef HAVE_NET
  // CPhipps - now wait for netgame start
  D_CheckNetGame();
#endif

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"R_Init: Init DOOM refresh daemon - ");
  R_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"\nP_Init: Init Playloop state.\n");
  P_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"I_Init: Setting up machine state.\n");
  I_Init();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"S_Init: Setting up sound.\n");
  S_Init(snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"HU_Init: Setting up heads up display.\n");
  HU_Init();

  if (!(M_CheckParm("-nodraw") && M_CheckParm("-nosound")))
    I_InitGraphics();

  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"ST_Init: Init status bar.\n");
  ST_Init();

  idmusnum = -1; //jff 3/17/98 insure idmus number is blank

  // CPhipps - auto screenshots
  if ((p = M_CheckParm("-autoshot")) && (p < myargc-2))
    if ((auto_shot_count = auto_shot_time = atoi(myargv[p+1])))
      auto_shot_fname = myargv[p+2];

  // start the apropriate game based on parms

  // killough 12/98:
  // Support -loadgame with -record and reimplement -recordfrom.

  if ((slot = M_CheckParm("-recordfrom")) && (p = slot+2) < myargc)
    G_RecordDemo(myargv[p]);
  else
    {
      slot = M_CheckParm("-loadgame");
      if ((p = M_CheckParm("-record")) && ++p < myargc)
  {
    autostart = true;
    G_RecordDemo(myargv[p]);
  }
    }

  if ((p = M_CheckParm ("-checksum")) && ++p < myargc)
    {
      P_RecordChecksum (myargv[p]);
    }

  if ((p = M_CheckParm ("-fastdemo")) && ++p < myargc)
    {                                 // killough
      fastdemo = true;                // run at fastest speed possible
      timingdemo = true;              // show stats after quit
      G_DeferedPlayDemo(myargv[p]);
      singledemo = true;              // quit after one demo
    }
  else
    if ((p = M_CheckParm("-timedemo")) && ++p < myargc)
      {
  singletics = true;
  timingdemo = true;            // show stats after quit
  G_DeferedPlayDemo(myargv[p]);
  singledemo = true;            // quit after one demo
      }
    else
      if ((p = M_CheckParm("-playdemo")) && ++p < myargc)
  {
    G_DeferedPlayDemo(myargv[p]);
    singledemo = true;          // quit after one demo
  }

  if (slot && ++slot < myargc)
    {
      slot = atoi(myargv[slot]);        // killough 3/16/98: add slot info
      G_LoadGame(slot, true);           // killough 5/15/98: add command flag // cph - no filename
    }
  else
    if (!singledemo) {                  /* killough 12/98 */
      if (autostart || netgame)
  {
    G_InitNew(startskill, startepisode, startmap);
    if (demorecording)
      G_BeginRecording();
  }
      else
  D_StartTitle();                 // start up intro loop
    }
}

//
// D_DoomMain
//
// Not called in iphone version.
/*
void D_DoomMain(void)
{
  D_DoomMainSetup(); // CPhipps - setup out of main execution stack

  D_DoomLoop ();  // never returns
}
*/

//
// GetFirstMap
//
// Ty 08/29/98 - determine first available map from the loaded wads and run it
//

void GetFirstMap(int *ep, int *map)
{
  int i,j; // used to generate map name
  boolean done = false;  // Ty 09/13/98 - to exit inner loops
  char test[6];  // MAPxx or ExMx plus terminator for testing
  char name[6];  // MAPxx or ExMx plus terminator for display
  boolean newlevel = false;  // Ty 10/04/98 - to test for new level
  int ix;  // index for lookup

  strcpy(name,""); // initialize
  if (*map == 0) // unknown so go search for first changed one
  {
    *ep = 1;
    *map = 1; // default E1M1 or MAP01
    if (gamemode == commercial)
    {
      for (i=1;!done && i<33;i++)  // Ty 09/13/98 - add use of !done
      {
        sprintf(test,"MAP%02d",i);
        ix = W_CheckNumForName(test);
        if (ix != -1)  // Ty 10/04/98 avoid -1 subscript
        {
          if (lumpinfo[ix].source == source_pwad)
          {
            *map = i;
            strcpy(name,test);  // Ty 10/04/98
            done = true;  // Ty 09/13/98
            newlevel = true; // Ty 10/04/98
          }
          else
          {
            if (!*name)  // found one, not pwad.  First default.
               strcpy(name,test);
          }
        }
      }
    }
    else // one of the others
    {
      strcpy(name,"E1M1");  // Ty 10/04/98 - default for display
      for (i=1;!done && i<5;i++)  // Ty 09/13/98 - add use of !done
      {
        for (j=1;!done && j<10;j++)  // Ty 09/13/98 - add use of !done
        {
          sprintf(test,"E%dM%d",i,j);
          ix = W_CheckNumForName(test);
          if (ix != -1)  // Ty 10/04/98 avoid -1 subscript
          {
            if (lumpinfo[ix].source == source_pwad)
            {
              *ep = i;
              *map = j;
              strcpy(name,test); // Ty 10/04/98
              done = true;  // Ty 09/13/98
              newlevel = true; // Ty 10/04/98
            }
            else
            {
              if (!*name)  // found one, not pwad.  First default.
                 strcpy(name,test);
            }
          }
        }
      }
    }
    //jff 9/3/98 use logical output routine
    lprintf(LO_CONFIRM,"Auto-warping to first %slevel: %s\n",
      newlevel ? "new " : "", name);  // Ty 10/04/98 - new level test
  }
}
