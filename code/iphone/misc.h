/*
 *  misc.h
 *  doom
 *
 *  Created by John Carmack on 4/13/09.
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


char *va( const char *format, ... );
void Com_Printf( const char *fmt, ... );
void Com_Error( const char *fmt, ... );
int HashString( const char *string );

/*
 
 Command execution takes a NUL-terminated string, breaks it into tokens,
 then searches for a command or variable that matches the first token.
 
 */

typedef void (*xcommand_t) (void);

// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function );

// print all the added commands
void	Cmd_ListCommands_f();

// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits
char	*Cmd_CompleteCommand( const char *partial );

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.
int		Cmd_Argc( void );
const char	*Cmd_Argv( int arg );

// Takes a NUL-terminated string.  Does not need to be /n terminated.
// breaks the string up into argc / argv tokens.
void	Cmd_TokenizeString( const char *text );

// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console
void	Cmd_ExecuteString( const char *text );

// execute each line of the config file
void	Cmd_ExecuteFile( const char *fullPathName );

