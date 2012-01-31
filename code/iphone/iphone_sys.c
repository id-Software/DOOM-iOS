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


#include "../doomiphone.h"
#import <AudioToolbox/AudioServices.h>

int SysIphoneMicroseconds() {
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;
	
	gettimeofday( &tp, &tzp );
	
	if( ! secbase ) {
		secbase = tp.tv_sec;
		return tp.tv_usec;
	}
	
	int curtime = (tp.tv_sec - secbase) * 1000000 + tp.tv_usec;
	
	return curtime;
}

int SysIphoneMilliseconds() {
	return SysIphoneMicroseconds()/1000;
}


extern char iphoneDocDirectory[1024];
extern char iphoneAppDirectory[1024];

const char *SysIphoneGetAppDir() {
	return iphoneAppDirectory;
}

const char *SysIphoneGetDocDir() {
	return iphoneDocDirectory;
}

void SysIPhoneVibrate() {
	AudioServicesPlaySystemSound( kSystemSoundID_Vibrate );
}


