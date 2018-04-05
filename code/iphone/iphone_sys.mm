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
#import <AudioToolbox/AudioServices.h>
#import <UIKit/UIKit.h>
#import "../ios/View.h"

char	consoleCommand[1024];

int SysIphoneMicroseconds() {
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;
	
	gettimeofday( &tp, &tzp );
	
	if( ! secbase ) {
		secbase = (int) tp.tv_sec;
		return tp.tv_usec;
	}
	
	int curtime = (int)((tp.tv_sec - secbase) * 1000000 + tp.tv_usec);
	
	return curtime;
}

int SysIphoneMilliseconds() {
	return SysIphoneMicroseconds()/1000;
}


char iphoneDocDirectory[PATH_MAX];
char iphoneAppDirectory[PATH_MAX];
char iphoneTempDirectory[PATH_MAX];

const char *SysIphoneGetAppDir() {
	return iphoneAppDirectory;
}

const char *SysIphoneGetDocDir() {
	return iphoneDocDirectory;
}

const char *SysIphoneGetTempDir() {
	return iphoneTempDirectory;
}

void SysIPhoneVibrate() {
	AudioServicesPlaySystemSound( kSystemSoundID_Vibrate );
}


void SysIPhoneOpenURL( const char *url ) {
	Com_Printf( "OpenURL char *: %s\n", url );
	
	NSString *nss = [NSString stringWithCString: url encoding: NSASCIIStringEncoding];
	[[UIApplication sharedApplication] openURL:[NSURL URLWithString: nss]];
}

int SysIPhoneIsDeviceLandscapeRight( void ) {
	return [UIDevice currentDevice].orientation == UIDeviceOrientationLandscapeRight;
}

void SysIPhoneSetUIKitOrientation( int isLandscapeRight ) {
	// Using OS autorotation for now. If we ever want to support 60FPS on MBX devices,
	// we'll probably have to go back to manual rotation for the GL view.
	/*
	if ( isLandscapeRight ) {
		[UIApplication sharedApplication].statusBarOrientation = UIInterfaceOrientationLandscapeRight;
	} else {
		[UIApplication sharedApplication].statusBarOrientation = UIInterfaceOrientationLandscapeLeft;
	}
	*/
}
