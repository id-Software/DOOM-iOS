/*
 =======================================================================================
 
 Copyright (C) 2009-2011 id Software LLC, a ZeniMax Media company.  All Right Reserved.
 
 This file is part of the DOOM Classic iOS v2.1 GPL Source Code.
 
 =======================================================================================
 */



#import <Foundation/Foundation.h>


@interface UICustomSwitch : UIButton {
    BOOL on;
}

@property(nonatomic,getter=isOn) BOOL on;

@end
