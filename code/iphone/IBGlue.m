/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#include "IBGlue.h"
#import "doomAppDelegate.h"
#include "doomiphone.h"

// Tells Interface Builder to go to the Main Menu.
void IB_GotoMainMenu() {
    [ gAppDelegate MainMenu];
}
