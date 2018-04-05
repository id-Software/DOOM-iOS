/*
 *  doomiphone.h
 *  doom
 *
 *  Created by John Carmack on 3/13/09.
 *  Copyright 2009 idSoftware. All rights reserved.
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

#ifndef _DOOM_IPHONE_H_
#define _DOOM_IPHONE_H_

#define SCREEN_WIDTH ([[UIScreen mainScreen] bounds].size.width)
#define SCREEN_HEIGHT ([[UIScreen mainScreen] bounds].size.height)
#define SCREEN_MAX_LENGTH (MAX(SCREEN_WIDTH, SCREEN_HEIGHT))
#define SCREEN_MIN_LENGTH (MIN(SCREEN_WIDTH, SCREEN_HEIGHT))
#define IS_IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
#define IS_IPHONE (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
#define IS_IPHONE_X (IS_IPHONE && SCREEN_MAX_LENGTH == 812.0)
#define IPHONE_X_BOTTOM_INSET 64

#define PWAD_LIST_SEPARATOR ':'

#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <setjmp.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "gles_glue.h"

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/oalStaticBufferExtension.h>
#undef ALCAPI
#define ALCAPI

#ifdef __cplusplus
extern "C" {
#endif

#include "prboom/SDL_opengl.h"

// prBoom code
#include "prboom/m_fixed.h"
#include "prboom/doomdef.h"
#include "prboom/doomtype.h"
#include "prboom/doomstat.h"
#include "prboom/d_net.h"
#include "prboom/dstrings.h"
#include "prboom/sounds.h"
#include "prboom/z_zone.h"
#include "prboom/w_wad.h"
#include "prboom/s_sound.h"
#include "prboom/v_video.h"
#include "prboom/f_finale.h"
#include "prboom/f_wipe.h"
#include "prboom/m_argv.h"
#include "prboom/m_misc.h"
#include "prboom/m_menu.h"
#include "prboom/p_checksum.h"
#include "prboom/i_main.h"
#include "prboom/i_system.h"
#include "prboom/i_sound.h"
#include "prboom/i_video.h"
#include "prboom/g_game.h"
#include "prboom/hu_stuff.h"
#include "prboom/wi_stuff.h"
#include "prboom/st_stuff.h"
#include "prboom/am_map.h"
#include "prboom/p_setup.h"
#include "prboom/r_draw.h"
#include "prboom/r_main.h"
#include "prboom/r_fps.h"
#include "prboom/d_main.h"
#include "prboom/d_deh.h"
#include "prboom/lprintf.h"
#include "prboom/am_map.h"
#include "prboom/gl_intern.h"
#include "prboom/p_mobj.h"
#include "prboom/p_maputl.h"
#include "prboom/p_map.h"
// open / close name collision problem... #include "prboom/p_spec.h"
#include "prboom/p_inter.h"
#include "prboom/m_random.h"
#include "prboom/m_bbox.h"
#include "prboom/m_cheat.h"

// we will now define landscapeViewport / landscapeScissor to rotate the coords
#undef glViewport
#undef glScissor

// our vestigial system environment
#include "misc.h"
#include "cvar.h"

// new iphone code
#include "ipak.h"
#include "iphone_email.h" //gsh, adds support for emailing the console to id
    
#ifdef __cplusplus
}
#endif

#include "iphone_doom.h"

#endif 
