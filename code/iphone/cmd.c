/*
 *  cmd.c
 *  doom
 *
 *  Created by John Carmack on 4/14/09.
 *  Copyright 2009 id Software. All rights reserved.
 *
 */
/*
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

typedef struct cmd_function_s {
	struct cmd_function_s	*next;
	const char				*name;
	int						hashid;
	xcommand_t				function;
} cmd_function_t;


#define MAX_STRING_TOKENS	16
#define MAX_STRING_CHARS	1024

int			cmd_argc;
char		*cmd_argv[ MAX_STRING_TOKENS ];

cmd_function_t	*cmd_functions;		// possible commands to execute

int Cmd_Argc( void ) {
	return cmd_argc;
}

const char *Cmd_Argv( int arg ) {
	if( arg >= cmd_argc ) {
		return "";
	}
	
	return cmd_argv[ arg ];	
}


void Cmd_TokenizeString( const char *text ) {
	static char *stringCopy;
	
	// clear the args from the last string
	// This better not be called recursively...
	if ( stringCopy ) {
		free( stringCopy );
		stringCopy = NULL;
	}
	
	cmd_argc = 0;
	
	if( ! text ) {
		return;
	}
	
	stringCopy = strdup( text );
	char *strval = stringCopy;
	
	while( 1 ) {
		char *start = strsep( &strval," \t\r\n");
		if ( !start ) {
			break;
		}
		if ( start[0] != 0 ) {
			cmd_argv[cmd_argc] = start;
			if ( ++cmd_argc == MAX_STRING_TOKENS ) {
				break;
			}
		}
	}
}

void Cmd_ListCommands_f() {
	for( cmd_function_t *cmd = cmd_functions ; cmd ; cmd = cmd->next ) {
		Com_Printf( "%s\n", cmd->name );
	}
}

void Cmd_AddCommand( const char *cmd_name, xcommand_t function ) {
	cmd_function_t	*cmd;
	int hashid;
	
	hashid = HashString( cmd_name );
	
	// fail if the command already exists
	for( cmd = cmd_functions ; cmd ; cmd = cmd->next ) {
		if( hashid == cmd->hashid && !strcmp( cmd_name, cmd->name ) ) {
			Com_Printf( "Cmd_AddCommand: \"%s\" already defined\n", cmd_name );
			return;
		}
	}
	
	cmd = malloc( sizeof( cmd_function_t ) );
	cmd->name = cmd_name;
	cmd->hashid = hashid;
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
	
}

void	Cmd_ExecuteString( const char *str ) {	
	int l = (int) strlen( str );
	if ( str[l-1] == '\n' ) {
		char *stripped = alloca( l+1 );
		strcpy( stripped, str );
		str = stripped;
		stripped[l-1] = 0;
	}
	
	Com_Printf( "%s\n", str );
	Cmd_TokenizeString( str );
		 
	const char *arg0 = Cmd_Argv( 0 );
	int hashid = HashString( arg0 );
	
	// check commands first
	for( cmd_function_t *cmd = cmd_functions ; cmd ; cmd = cmd->next ) {
		if( hashid == cmd->hashid && !strcmp( arg0, cmd->name ) ) {
			cmd->function();
			return;
		}
	}
	
	// then check cvars
	cvar_t *cvar = Cvar_FindVar( arg0 );
	if ( cvar ) {
		Cvar_Set( arg0, Cmd_Argv( 1 ) );
		return;
	}
	Com_Printf( "Unknown command: %s\n", arg0 );
}

// execute each line of the config file
void	Cmd_ExecuteFile( const char *fullPathName ) {
	Com_Printf( "Executing command file '%s'\n", fullPathName );
	FILE *f = fopen( fullPathName, "rb" );
	if ( !f ) {
		Com_Printf( "Failed to open.\n" );
		return;
	}
	char	line[1024];
	while( fgets( line, sizeof( line ), f ) ) {
		Cmd_ExecuteString( line );
	}
	fclose( f );
}

