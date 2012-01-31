/*
    Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
	Copyright (C) 2004 Michael Liebscher
	Copyright (C) 1997-2001 Id Software, Inc.

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

/*
 *	cvar.h: Dynamic variable tracking.
 *	
 *	Author:	Michael Liebscher	<johnnycanuck@users.sourceforge.net>	    
 *	Date:	2004   
 *
 *	Acknowledgement:
 *	This code was derived from Quake II, and was originally
 *	written by Id Software, Inc.
 *
 */

/*

Notes:

	Dynamic variable tracking.

	cvar_t variables are used to hold scalar or string variables 
	that can be changed or displayed at the console or prog code 
	as well as accessed directly in C code.

	The user can access cvars from the console in three ways:
	r_draworder			-prints the current value
	r_draworder 0		-sets the current value to 0
	set r_draworder 0	-as above, but creates the cvar if not present
	
	Cvars are restricted from having the same names as commands to keep this
	module from being ambiguous.

	This module is implemented by cvar.c

*/

#ifndef __CVAR_H__
#define	__CVAR_H__

typedef enum _CVARFlags 
{
	CVAR_INIT		= 0x0,	// Just create it with no flag value.	
    CVAR_ARCHIVE 	= 0x1,	// Set to cause it to be saved to the config file
    CVAR_NOSET		= 0x8,	// Don't allow change from console at all,
							// but can be set from the command line.
} CVARFlags;


// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	// By putting the value first, cvars can be referenced in other code
	// as just extern float *name, without having to include the cvar_t
	// declaration.  This is probably a bad idea that I will
	// regret at some point.
	float		value;
	
	char		*name;
	char		*string;
	char		*defaultString;
	int			hashid;
	int			flags;
	boolean		modified;	// set each time the cvar is changed
	struct cvar_s *next;
} cvar_t;

extern	cvar_t	*cvar_vars;

void Cvar_List_f();
void Cvar_Reset_f();

extern cvar_t *Cvar_Get( const char *var_name, const char *value, CVARFlags flags );
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

extern cvar_t *Cvar_FindVar( const char *var_name );
// returns NULL if it doesn't exist

extern void Cvar_Set( const char *var_name, const char *value );
// prints warning if it doesn't exist

extern void	Cvar_SetValue( const char *var_name, float value );
// expands value to a string and calls Cvar_Set

extern float Cvar_VariableValue( const char *var_name );
// returns 0 if not defined or non numeric

extern char *Cvar_VariableString( const char *var_name );
// returns an empty string if not defined

extern char *Cvar_CompleteVariable( const char *partial );
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

extern void Cvar_GetLatchedVars( void );
// any CVAR_LATCHED variables that have been set will now take effect

extern void Cvar_WriteVariables( const char *path );
// appends lines containing "set variable value" for all variables
// with the archive flag set to true.

extern void Cvar_Init( void );

extern char *Cvar_Userinfo( void );
// returns an info string containing all the CVAR_USERINFO cvars

extern char *Cvar_Serverinfo( void );
// returns an info string containing all the CVAR_SERVERINFO cvars

#endif /* __CVAR_H__ */

