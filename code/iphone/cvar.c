/*

	Copyright (C) 2004 Michael Liebscher
	Copyright (C) 1997-2001 Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General  License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General  License for more details.

	You should have received a copy of the GNU General  License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "doomiphone.h"


cvar_t	*cvar_vars;


/*
-----------------------------------------------------------------------------
 Function: Cvar_FindVar -Return cvar;
 
 Parameters: var_name -[in] Name of cvar to lookup.
 
 Returns: NULL if cvar not found, otherwise returns the cvar.
 
 Notes: 

-----------------------------------------------------------------------------
*/
cvar_t *Cvar_FindVar( const char *var_name )
{
	cvar_t	*var;
	int hashid;

	hashid = HashString( var_name );
	
	for( var = cvar_vars ; var ; var = var->next )
	{
		if( hashid == var->hashid && !strcasecmp( var_name, var->name ) )
		{
			return var;
		}
	}

	return NULL;
}

/*
-----------------------------------------------------------------------------
 Function: Cvar_VariableValue -Get value of cvar.
 
 Parameters: var_name -[in] Name of cvar to get value.
 
 Returns: 0 if not found, other the value of the cvar.
 
 Notes: 

-----------------------------------------------------------------------------
*/
 float Cvar_VariableValue( const char *var_name )
{
	cvar_t	*var;
	
	var = Cvar_FindVar( var_name );	
	if( ! var )
	{
		return 0;
	}

	return (float)atof( var->string );
}


/*
-----------------------------------------------------------------------------
 Function: Cvar_VariableString -Get cvar variable as string.
 
 Parameters: var_name -[in] Name of cvar to get value.
 
 Returns: Blank string on error, otherwise value string.
 
 Notes: 

-----------------------------------------------------------------------------
*/
 char *Cvar_VariableString( const char *var_name )
{
	cvar_t *var;
	
	var = Cvar_FindVar( var_name );
	if( ! var )
	{
		return "";
	}

	return var->string;
}

/*
-----------------------------------------------------------------------------
 Function: Cvar_CompleteVariable -Complete cvar string name.
 
 Parameters: partial -[in] Partial name of string to look up.
 
 Returns: NULL if partial string not found, otherwise the complete
			string name.
 
 Notes: 

-----------------------------------------------------------------------------
*/
 char *Cvar_CompleteVariable( const char *partial )
{
	cvar_t	*cvar;
	size_t	len;
	
	len = strlen( partial );
	
	if( ! len )
	{
		return NULL;
	}	

//
// Check partial match.
//
	for( cvar = cvar_vars ; cvar ; cvar = cvar->next )
	{
		if( ! strncmp( partial, cvar->name, len ) )
		{
			return cvar->name;
		}
	}

	return NULL;
}

/*
-----------------------------------------------------------------------------
 Function: Cvar_Get -Get cvar structure.
 
 Parameters:
			var_name -[in] the name of the cvar variable.
			var_value -[in] string value of the cvar variable.
			flags -[in] see CVARFlags for more information.
 
 Returns: NULL on error, otherwise valid pointer to cvar_t structure.
 
 Notes: 
	If the variable already exists, the value will not be set and
	the flags will be or'ed.
-----------------------------------------------------------------------------
*/
cvar_t *Cvar_Get( const char *var_name, const char *var_value, CVARFlags flags ) {
	cvar_t	*var;
	
	var = Cvar_FindVar( var_name );
	if( var ) {
		var->flags |= flags;
		return var;
	}

	if( ! var_value ) {
		return NULL;
	}

	var = malloc( sizeof( *var ) );
	var->name = strdup( var_name );
	var->string = strdup( var_value );
	var->defaultString = strdup( var_value );
	var->hashid = HashString( var_name );
	var->modified = true;
	var->value = (float)atof( var->string );

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	return var;
}

/*
-----------------------------------------------------------------------------
 Function: 
 
 Parameters:
 
 Returns:
 
 Notes: 

-----------------------------------------------------------------------------
*/
void Cvar_Set( const char *var_name, const char *value ) {
	cvar_t	*var;

	var = Cvar_FindVar( var_name );
	if( ! var ) {
		Com_Printf( "Cvar '%s' doesn't exist\n", var_name );
		return;
	}

	if( var->flags & CVAR_NOSET ) {
		Com_Printf( "%s is write protected.\n", var_name );
		return;
	}


	if( ! strcmp( value, var->string ) ) {
		return;		// not changed
	}

	var->modified = true;

	free( var->string );	// free the old value string
	
	var->string = strdup( value );
	var->value = (float)atof( var->string );
}


/*
-----------------------------------------------------------------------------
 Function: 
 
 Parameters:
 
 Returns:
 
 Notes: 

-----------------------------------------------------------------------------
*/
 void Cvar_SetValue( const char *var_name, float value )
{
	char	val[ 32 ];

	if( value == (int)value )
	{
		snprintf( val, sizeof( val ), "%i", (int)value );
	}
	else
	{
		snprintf( val, sizeof( val ), "%f", value );
	}
	
	Cvar_Set( var_name, val );
}



/*
-----------------------------------------------------------------------------
 Function: Cvar_Command -Handles variable inspection and changing from 
						the console.
 
 Parameters: Nothing.
 
 Returns: false if variable not found, otherwise true.
 
 Notes: 

-----------------------------------------------------------------------------
*/
boolean Cvar_Command( void )
{
	cvar_t	*v;

// check variables
	v = Cvar_FindVar( Cmd_Argv( 0 ) );
	if( ! v )
	{
		return false;
	}
		
// perform a variable print or set
	if( Cmd_Argc() == 1 )
	{
		Com_Printf( "\"%s\" is \"%s\"\n", v->name, v->string );
		return true;
	}

	Cvar_Set( v->name, Cmd_Argv( 1 ) );
	return true;
}


/*
-----------------------------------------------------------------------------
 Function: Cvar_WriteVariables -Appends lines containing "set variable value"
								for all variables with the archive flag set
								to true.
 
 Parameters:
 
 Returns: Nothing.
 
 Notes: 

-----------------------------------------------------------------------------
*/
 void Cvar_WriteVariables( const char *path )
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;

	f = fopen( path, "a" );
	for( var = cvar_vars ; var ; var = var->next )
	{
		if( var->flags & CVAR_ARCHIVE )
		{
			snprintf( buffer, sizeof( buffer ), "set %s %s\n", var->name, var->string );
			fprintf( f, "%s", buffer );
		}
	}
	fclose( f );
}

/*
-----------------------------------------------------------------------------
 Function: Cvar_List_f -Print all cvars to the console.
 
 Parameters: Nothing.
 
 Returns: Nothing.
 
 Notes: 

-----------------------------------------------------------------------------
*/
void Cvar_List_f( void )
{
	cvar_t	*var;
	int		i;
	
	i = 0;
	for( var = cvar_vars ; var ; var = var->next, ++i )
	{
		if( var->flags & CVAR_ARCHIVE )
		{
			Com_Printf ("*");
		}
		else
		{
			Com_Printf (" ");
		}
		
		
		Com_Printf (" %s \"%s\"\n", var->name, var->string);
	}
	
	Com_Printf ("%i cvars\n", i);
}


void Cvar_Reset_f( void ) {
	for( cvar_t *var = cvar_vars ; var ; var = var->next ) {
		Cvar_Set( var->name, var->defaultString );
	}
}

