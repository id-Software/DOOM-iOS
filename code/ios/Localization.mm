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

#include "Localization.h"
#include "LocalizationObjectiveC.h"

#import <Foundation/Foundation.h>

static NSBundle*        mCurrentBundle = NULL;
static ID_LANGUAGE      mCurrentLanguage;
static const char*      mCurrentLanguageStr;

/*
 ========================
 idLocalization::SetLanguage
 Sets the language of the system with an enumeration passed in.
 ========================
 */
void idLocalization_SetLanguage( ID_LANGUAGE language ) {
    
    NSString* langPath = nil;
    
    switch( language ) {
        case LANGUAGE_ENGLISH:
            langPath = [[NSBundle mainBundle ] pathForResource:@"en" ofType:@"lproj" ];
            break;
        case LANGUAGE_FRENCH:
            langPath = [[NSBundle mainBundle ] pathForResource:@"fr" ofType:@"lproj" ];
            break;
        case LANGUAGE_ITALIAN:
            langPath = [[NSBundle mainBundle ] pathForResource:@"it" ofType:@"lproj" ];
            break;
        case LANGUAGE_GERMAN:
            langPath = [[NSBundle mainBundle ] pathForResource:@"de" ofType:@"lproj" ];
            break;
        case LANGUAGE_SPANISH: 
            langPath = [[NSBundle mainBundle ] pathForResource:@"es" ofType:@"lproj" ];
            break;
        default:
            langPath = [[NSBundle mainBundle ] pathForResource:@"en" ofType:@"lproj" ];
            break;
    }
    
    mCurrentBundle = [[NSBundle bundleWithPath:langPath] retain ]; 
}


/*
 ========================
 idLocalization::GetLanguage
 Gets the current system's language
 ========================
 */
ID_LANGUAGE idLocalization_GetLanguage() {
    return mCurrentLanguage;
}

/*
 ========================
 idLocalization::Initialize
    Gathers the local device's language, and sets that as the systems language.
 ========================
 */
void idLocalization_Initialize() {
    
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSArray* languages = [defaults objectForKey:@"AppleLanguages"];
    NSString* current  = [[languages objectAtIndex:0] retain ];
    mCurrentLanguageStr = [current UTF8String];
    
    if([current isEqualToString:@"en"]){
        mCurrentLanguage = LANGUAGE_ENGLISH;
    } else if([current isEqualToString:@"fr"]){
        mCurrentLanguage = LANGUAGE_FRENCH;
    } else if([current isEqualToString:@"it"]){
        mCurrentLanguage = LANGUAGE_ENGLISH;
    } else if([current isEqualToString:@"sp"] || [current isEqualToString:@"es"]){
        mCurrentLanguage = LANGUAGE_SPANISH;
    } else if([current isEqualToString:@"ge"] || [current isEqualToString:@"de"]){
        mCurrentLanguage = LANGUAGE_GERMAN;
    } else {
        mCurrentLanguage = LANGUAGE_ENGLISH;
    }

    idLocalization_SetLanguage( mCurrentLanguage ); 
}

/*
 ========================
 idLocalization::GetString
    Gets a String value from the pair key passed in from Localizable.strings file.
 ========================
 */
const char * idLocalization_GetString( const char * key ) {
    
    NSString* nsKey = [NSString stringWithUTF8String:key];
    NSString* nsString = [mCurrentBundle localizedStringForKey:nsKey value:nil table:nil];
    const char* cString = nil;
    
    if( nsString != nil ) {
         cString = [nsString cStringUsingEncoding:NSWindowsCP1252StringEncoding];
    } else  {
        cString = key;
    }
    
    return cString;
}

/*
 ========================
 idLocalization::GetString
    Gets a String value from the pair key passed in from Localizable.strings file.
 ========================
 */
NSString * idLocalization_GetNSString( NSString * key ) {
	return [mCurrentBundle localizedStringForKey:key value:nil table:nil];
}
