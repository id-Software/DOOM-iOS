//
//  GameController.mm
//  doomengine
//
//  Created by John Watson on 1/29/15.
//
//

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

#import <GameController/GameController.h>

#import "doomiphone.h"

#import "GameController.h"

static GCController *controller = nil;
static bool togglePause = false;

typedef enum {
    WEAPON_PREVIOUS = 0,
    WEAPON_NEXT = 1,
}weaponswitch_e;

static bool HasAmmo(int weaponType) {
    // Trivial case: you've always got fist
    if(weaponType == wp_fist) {
        return true;
    }
    
    player_t *player = &players[consoleplayer];
    
    if(!player->weaponowned[weaponType]) {
        // Don't switch to an unowned weapon
        return false;
    }
    
    int ammo = -1;
    
    switch ( weaponType ) {
        case wp_pistol: ammo = player->ammo[am_clip]; break;
        case wp_shotgun: ammo = player->ammo[am_shell]; break;
        case wp_chaingun: ammo = player->ammo[am_clip]; break;
        case wp_missile: ammo = player->ammo[am_misl]; break;
        case wp_plasma: ammo = player->ammo[am_cell]; break;
        case wp_bfg: ammo = player->ammo[am_cell]; if ( ammo < 40 ) ammo = 0; break;
        case wp_supershotgun: ammo = player->ammo[am_shell]; if ( ammo < 2 ) ammo = 0; break;
    }
    
    return ammo > 0;
}

static int SwitchWeapon(weaponswitch_e direction) {
    int index = players[consoleplayer].readyweapon;
    if(direction == WEAPON_PREVIOUS) {
        --index;
        while(!HasAmmo(index)) {
            if(--index < 0) {
                index = NUMWEAPONS - 1;
            }
        }
    } else if(direction == WEAPON_NEXT) {
        ++index;
        while(!HasAmmo(index)) {
            if(++index >= NUMWEAPONS) {
                index = 0;
            }
        }
    }
    
    if(index == weaponSelected) {
        return wp_nochange;
    }
    
    return index;
}

static void setupPauseButtonHandler(GCController *ctrl) {
    if(ctrl) {
        ctrl.controllerPausedHandler = ^ (GCController* c){
            togglePause = true;
        };
    }
}

/**
 Lazily initialze game controller on first request
 return true if controller is available
 */
bool iphoneControllerIsAvailable() {
    static bool initialized = false;
    if(!initialized) {
        NSArray *controllers = [GCController controllers];
        if(controllers.count > 0) {
//            controller = controllers[0]; // Just use the first one
            
            // If we have neither gamepad nor extended gamepad support, just make controller nil
//            if(![controller gamepad] && ![controller extendedGamepad]) {
                controller = nil;
//            }
            
            for (int i = 0; i < controllers.count; i++)
            {
                if([controllers[i] gamepad] || [controllers[i] extendedGamepad]) {
                    controller = controllers[i];
                }
            }
            
#if !TARGET_OS_TV
            setupPauseButtonHandler(controller);
            
            // Register for controller connected/disconnected notifications
            NSNotificationCenter *ns = [NSNotificationCenter defaultCenter];
            [ns addObserverForName:GCControllerDidConnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
                if(!controller) {
                    controller = [GCController controllers][0];
                    if(![controller gamepad] && ![controller extendedGamepad]) {
                        controller = nil;
                    }
                    
                    setupPauseButtonHandler(controller);
                }
            }];
            
            [ns addObserverForName:GCControllerDidDisconnectNotification object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
                controller = nil;
            }];
#endif
        }
        initialized = true; // Only need to do this once
    }
    return controller != nil;
}

void iphoneControllerInput(ticcmd_t* cmd) {
    if(iphoneControllerIsAvailable()) {
        // Perform standard gamepad updates
        GCExtendedGamepad *gamepad = controller.extendedGamepad;
        
        cmd->angleturn += (gamepad.rightThumbstick.xAxis.value * -ROTATETHRESHOLD);
        cmd->forwardmove += (gamepad.leftThumbstick.yAxis.value * TURBOTHRESHOLD);
        cmd->sidemove += (gamepad.leftThumbstick.xAxis.value * TURBOTHRESHOLD);
        
        if(gamepad.rightTrigger.pressed) {
            cmd->buttons |= BT_ATTACK;
        }
        
        if(gamepad.buttonA.pressed) {
            cmd->buttons |= BT_USE;
        }
        
        int newWeapon = wp_nochange;
        
        // Switch weapons using X and Y buttons
        if(gamepad.buttonX.pressed && players[consoleplayer].pendingweapon == wp_nochange) {
            newWeapon = SwitchWeapon(WEAPON_PREVIOUS);
        }
        
        if(gamepad.buttonY.pressed && players[consoleplayer].pendingweapon == wp_nochange) {
            newWeapon = SwitchWeapon(WEAPON_NEXT);
        }
        
        // If we switched weapons, pass that info down to our frame cmd
        if(newWeapon != wp_nochange) {
            cmd->buttons |= BT_CHANGE;
            cmd->buttons |= newWeapon << BT_WEAPONSHIFT;
        }
        
//        if(gamepad.dpad.left.pressed && players[consoleplayer].playerstate == PST_DEAD) {
//            cmd->buttons |= BT_USE;
//        }
//        
//        if(gamepad.dpad.up.pressed && players[consoleplayer].playerstate == PST_DEAD) {
//            cmd->buttons |= BT_SPECIAL;
//        }
//        
//        if(gamepad.dpad.right.pressed && players[consoleplayer].playerstate == PST_DEAD) {
//            cmd->buttons |= BT_SPECIALMASK;
//        }

        if(togglePause) {
            cmd->buttons |= BT_SPECIAL | (BTS_PAUSE & BT_SPECIALMASK);
            togglePause = false;
        }
    }
}
