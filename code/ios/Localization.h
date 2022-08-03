/*
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.
 
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
 ================================================================================================
 
 A game-generic C++ wrapper around the iOS Localizable Strings functionality.
 
 Basics of Localaization on Apple platforms:
    
    Each Localization language has a bundle associated with it  ( *.lproj ).
    We gather the bundle for the current ( or needed ) language. 
        [[NSBundle mainBundle ] pathForResource:@"en" ofType:@"lproj" ];
 
    You MUST add a Localizable.strings file ( named exactly that ) to the project.
    
    Apple provides a system for getting strings from the localizable.strings file.
        NSString* nsString = [ bundle localizedStringForKey:key value:nil table:nil ];
    
    The Localizable.strings file must be formatted like:
        "KEY" = "VALUE"; 
    and end with a semi-coln. 
 
 Apple Developer Reference: 
 http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/LoadingResources/Strings/Strings.html
 
 We wrapper this up into 4 easy functions. 
    "Localization_Initialize" Must be called to create/set the current language bundle for the system.
    "Localization_SetLanguage" Can be called at dynamically to set the language. 
    "Localization_GetString" Gets a string value from a key'd pair
 
 SUPPORTED LANGUAGES: 
        ENGLISH,
        FRENCH,
        ITALIAN,
        GERMAN,
        SPANISH
 
 ================================================================================================
 */

#ifndef IDMOBILELIB_LOCALIZATION_H
#define IDMOBILELIB_LOCALIZATION_H

#ifdef __cplusplus 
    extern "C" {
#endif 
        
    enum ID_LANGUAGE {
            LANGUAGE_ENGLISH = 0,
            LANGUAGE_FRENCH, 
            LANGUAGE_ITALIAN,
            LANGUAGE_GERMAN,
            LANGUAGE_SPANISH
    };

    void             idLocalization_Initialize();
    void             idLocalization_SetLanguage(  enum ID_LANGUAGE language );
    enum ID_LANGUAGE idLocalization_GetLanguage();
    const char *     idLocalization_GetString( const char * key );

#ifdef __cplusplus 
        }
#endif 

#endif
