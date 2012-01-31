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

void Com_Printf( const char *fmt, ... ) {
	va_list		argptr;
	
	va_start( argptr, fmt );
	
	//gsh, send output to the console buffer
	char buffer[1024];
	vsnprintf( buffer, sizeof( buffer ), fmt, argptr );
	AppendConsoleBuffer(buffer);
	
	vprintf( fmt, argptr );
	va_end( argptr );
}

void Com_Error( const char *fmt, ... ) {
	va_list		argptr;
	
	va_start( argptr, fmt );
	
	//gsh, send output to the console buffer
	char buffer[1024];
	vsnprintf( buffer, sizeof( buffer ), fmt, argptr );
	AppendConsoleBuffer(buffer);
	
	vprintf( fmt, argptr );
	va_end( argptr );
	
	//gsh, email the console to id
	EmailConsole();
	
	// drop into the editor
	abort(); 
	exit( 1 );
}

char *va( const char *format, ... ) {
	va_list	argptr;
	static char	string[ 1024 ];
	
	va_start( argptr, format );
	(void)vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );
	
	string[ sizeof( string ) - 1 ] = '\0';
	
	return string;	
}

int HashString( const char *string ) {
	int hash = *string;
	
	if( hash ) {
		for( string += 1; *string != '\0'; ++string ) {
			hash = (hash << 5) - hash + tolower(*string);
		}
	}
	
	return hash;
}

