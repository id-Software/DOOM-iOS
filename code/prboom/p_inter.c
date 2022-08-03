/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Handling interactions (i.e., collisions).
 *
 *-----------------------------------------------------------------------------*/

#include "doomstat.h"
#include "dstrings.h"
#include "m_random.h"
#include "am_map.h"
#include "r_main.h"
#include "s_sound.h"
#include "sounds.h"
#include "d_deh.h"  // Ty 03/22/98 - externalized strings
#include "p_tick.h"
#include "lprintf.h"

#include "p_inter.h"
#include "p_enemy.h"

#ifdef __GNUG__
#pragma implementation "p_inter.h"
#endif
#include "p_inter.h"

#include "doomiphone.h"

#define BONUSADD        6

// Ty 03/07/98 - add deh externals
// Maximums and such were hardcoded values.  Need to externalize those for
// dehacked support (and future flexibility).  Most var names came from the key
// strings used in dehacked.

int initial_health = 100;
int initial_bullets = 50;
int maxhealth = 100; // was MAXHEALTH as a #define, used only in this module
int max_armor = 200;
int green_armor_class = 1;  // these are involved with armortype below
int blue_armor_class = 2;
int max_soul = 200;
int soul_health = 100;
int mega_health = 200;
int god_health = 100;   // these are used in cheats (see st_stuff.c)
int idfa_armor = 200;
int idfa_armor_class = 2;
// not actually used due to pairing of cheat_k and cheat_fa
int idkfa_armor = 200;
int idkfa_armor_class = 2;

int bfgcells = 40;      // used in p_pspr.c
int monsters_infight = 0; // e6y: Dehacked support - monsters infight
// Ty 03/07/98 - end deh externals

// a weapon is found with two clip loads,
// a big item has five clip loads
int maxammo[NUMAMMO]  = {200, 50, 300, 50};
int clipammo[NUMAMMO] = { 10,  4,  20,  1};

//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

static boolean P_GiveAmmo(player_t *player, ammotype_t ammo, int num)
{
  int oldammo;

  if (ammo == am_noammo)
    return false;

#ifdef RANGECHECK
  if (ammo > NUMAMMO)
    I_Error ("P_GiveAmmo: bad type %i", ammo);
#endif

  if ( player->ammo[ammo] == player->maxammo[ammo]  )
    return false;

  if (num)
    num *= clipammo[ammo];
  else
    num = clipammo[ammo]/2;

  // give double ammo in trainer mode, you'll need in nightmare
  if (gameskill == sk_baby || gameskill == sk_nightmare)
    num <<= 1;

  oldammo = player->ammo[ammo];
  player->ammo[ammo] += num;

  if (player->ammo[ammo] > player->maxammo[ammo])
    player->ammo[ammo] = player->maxammo[ammo];

  // If non zero ammo, don't change up weapons, player was lower on purpose.
  if (oldammo)
    return true;

  // We were down to zero, so select a new weapon.
  // Preferences are not user selectable.

  switch (ammo)
    {
    case am_clip:
      if (player->readyweapon == wp_fist) {
        if (player->weaponowned[wp_chaingun])
          player->pendingweapon = wp_chaingun;
        else
          player->pendingweapon = wp_pistol;
      }
      break;

    case am_shell:
      if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
        if (player->weaponowned[wp_shotgun])
          player->pendingweapon = wp_shotgun;
        break;

      case am_cell:
        if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
          if (player->weaponowned[wp_plasma])
            player->pendingweapon = wp_plasma;
        break;

      case am_misl:
        if (player->readyweapon == wp_fist)
          if (player->weaponowned[wp_missile])
            player->pendingweapon = wp_missile;
    default:
      break;
    }
  return true;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//

static boolean P_GiveWeapon(player_t *player, weapontype_t weapon, boolean dropped)
{
  boolean gaveammo;
  boolean gaveweapon;

  if (netgame && deathmatch!=2 && !dropped)
    {
      // leave placed weapons forever on net games
      if (player->weaponowned[weapon])
        return false;

      player->bonuscount += BONUSADD;
      player->weaponowned[weapon] = true;

      P_GiveAmmo(player, weaponinfo[weapon].ammo, deathmatch ? 5 : 2);

      player->pendingweapon = weapon;
      /* cph 20028/10 - for old-school DM addicts, allow old behavior
       * where only consoleplayer's pickup sounds are heard */
      // displayplayer, not consoleplayer, for viewing multiplayer demos
      if (!comp[comp_sound] || player == &players[displayplayer])
        S_StartSound (player->mo, sfx_wpnup|PICKUP_SOUND); // killough 4/25/98
      return false;
    }

  if (weaponinfo[weapon].ammo != am_noammo)
    {
      // give one clip with a dropped weapon,
      // two clips with a found weapon
      gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, dropped ? 1 : 2);
    }
  else
    gaveammo = false;

  if (player->weaponowned[weapon])
    gaveweapon = false;
  else
    {
      gaveweapon = true;
      player->weaponowned[weapon] = true;
      player->pendingweapon = weapon;
    }
  return gaveweapon || gaveammo;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//

static boolean P_GiveBody(player_t *player, int num)
{
  if (player->health >= maxhealth)
    return false; // Ty 03/09/98 externalized MAXHEALTH to maxhealth
  player->health += num;
  if (player->health > maxhealth)
    player->health = maxhealth;
  player->mo->health = player->health;
  return true;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//

static boolean P_GiveArmor(player_t *player, int armortype)
{
  int hits = armortype*100;
  if (player->armorpoints >= hits)
    return false;   // don't pick up
  player->armortype = armortype;
  player->armorpoints = hits;
  return true;
}

//
// P_GiveCard
//

static void P_GiveCard(player_t *player, card_t card)
{
  if (player->cards[card])
    return;
  player->bonuscount = BONUSADD;
  player->cards[card] = 1;
}

//
// P_GivePower
//
// Rewritten by Lee Killough
//

boolean P_GivePower(player_t *player, int power)
{
  static const int tics[NUMPOWERS] = {
    INVULNTICS, 1 /* strength */, INVISTICS,
    IRONTICS, 1 /* allmap */, INFRATICS,
   };

  switch (power)
    {
      case pw_invisibility:
        player->mo->flags |= MF_SHADOW;
        break;
      case pw_allmap:
        if (player->powers[pw_allmap])
          return false;
        break;
      case pw_strength:
        P_GiveBody(player,100);
        break;
    }

  // Unless player has infinite duration cheat, set duration (killough)

  if (player->powers[power] >= 0)
    player->powers[power] = tics[power];
  return true;
}

//
// P_TouchSpecialThing
//

void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher)
{
  player_t *player;
  int      i;
  int      sound;
  fixed_t  delta = special->z - toucher->z;

  if (delta > toucher->height || delta < -8*FRACUNIT)
    return;        // out of reach

  sound = sfx_itemup;
  player = toucher->player;

  // Dead thing touching.
  // Can happen with a sliding player corpse.
  if (toucher->health <= 0)
    return;

    // Identify by sprite.
  switch (special->sprite)
    {
      // armor
    case SPR_ARM1:
      if (!P_GiveArmor (player, green_armor_class))
        return;
      player->message = s_GOTARMOR; // Ty 03/22/98 - externalized
      break;

    case SPR_ARM2:
      if (!P_GiveArmor (player, blue_armor_class))
        return;
      player->message = s_GOTMEGA; // Ty 03/22/98 - externalized
      break;

        // bonus items
    case SPR_BON1:
      player->health++;               // can go over 100%
      if (player->health > (maxhealth * 2))
        player->health = (maxhealth * 2);
      player->mo->health = player->health;
      player->message = s_GOTHTHBONUS; // Ty 03/22/98 - externalized
      break;

    case SPR_BON2:
      player->armorpoints++;          // can go over 100%
      if (player->armorpoints > max_armor)
        player->armorpoints = max_armor;
      if (!player->armortype)
        player->armortype = green_armor_class;
      player->message = s_GOTARMBONUS; // Ty 03/22/98 - externalized
      break;

    case SPR_SOUL:
      player->health += soul_health;
      if (player->health > max_soul)
        player->health = max_soul;
      player->mo->health = player->health;
      player->message = s_GOTSUPER; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_MEGA:
      if (gamemode != commercial)
        return;
      player->health = mega_health;
      player->mo->health = player->health;
      P_GiveArmor (player,blue_armor_class);
      player->message = s_GOTMSPHERE; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

        // cards
        // leave cards for everyone
    case SPR_BKEY:
      if (!player->cards[it_bluecard])
        player->message = s_GOTBLUECARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_bluecard);
      if (!netgame)
        break;
      return;

    case SPR_YKEY:
      if (!player->cards[it_yellowcard])
        player->message = s_GOTYELWCARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_yellowcard);
      if (!netgame)
        break;
      return;

    case SPR_RKEY:
      if (!player->cards[it_redcard])
        player->message = s_GOTREDCARD; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_redcard);
      if (!netgame)
        break;
      return;

    case SPR_BSKU:
      if (!player->cards[it_blueskull])
        player->message = s_GOTBLUESKUL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_blueskull);
      if (!netgame)
        break;
      return;

    case SPR_YSKU:
      if (!player->cards[it_yellowskull])
        player->message = s_GOTYELWSKUL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_yellowskull);
      if (!netgame)
        break;
      return;

    case SPR_RSKU:
      if (!player->cards[it_redskull])
        player->message = s_GOTREDSKULL; // Ty 03/22/98 - externalized
      P_GiveCard (player, it_redskull);
      if (!netgame)
        break;
      return;

      // medikits, heals
    case SPR_STIM:
      if (!P_GiveBody (player, 10))
        return;
      player->message = s_GOTSTIM; // Ty 03/22/98 - externalized
      break;

    case SPR_MEDI:
      if (!P_GiveBody (player, 25))
        return;

      if (player->health < 50) // cph - 25 + the 25 just added, thanks to Quasar for reporting this bug
        player->message = s_GOTMEDINEED; // Ty 03/22/98 - externalized
      else
        player->message = s_GOTMEDIKIT; // Ty 03/22/98 - externalized
      break;


      // power ups
    case SPR_PINV:
      if (!P_GivePower (player, pw_invulnerability))
        return;
      player->message = s_GOTINVUL; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PSTR:
      if (!P_GivePower (player, pw_strength))
        return;
      player->message = s_GOTBERSERK; // Ty 03/22/98 - externalized
      if (player->readyweapon != wp_fist)
        player->pendingweapon = wp_fist;
      sound = sfx_getpow;
      break;

    case SPR_PINS:
      if (!P_GivePower (player, pw_invisibility))
        return;
      player->message = s_GOTINVIS; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_SUIT:
      if (!P_GivePower (player, pw_ironfeet))
        return;
      player->message = s_GOTSUIT; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PMAP:
      if (!P_GivePower (player, pw_allmap))
        return;
      player->message = s_GOTMAP; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

    case SPR_PVIS:
      if (!P_GivePower (player, pw_infrared))
        return;
      player->message = s_GOTVISOR; // Ty 03/22/98 - externalized
      sound = sfx_getpow;
      break;

      // ammo
    case SPR_CLIP:
      if (special->flags & MF_DROPPED)
        {
          if (!P_GiveAmmo (player,am_clip,0))
            return;
        }
      else
        {
          if (!P_GiveAmmo (player,am_clip,1))
            return;
        }
      player->message = s_GOTCLIP; // Ty 03/22/98 - externalized
      break;

    case SPR_AMMO:
      if (!P_GiveAmmo (player, am_clip,5))
        return;
      player->message = s_GOTCLIPBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_ROCK:
      if (!P_GiveAmmo (player, am_misl,1))
        return;
      player->message = s_GOTROCKET; // Ty 03/22/98 - externalized
      break;

    case SPR_BROK:
      if (!P_GiveAmmo (player, am_misl,5))
        return;
      player->message = s_GOTROCKBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_CELL:
      if (!P_GiveAmmo (player, am_cell,1))
        return;
      player->message = s_GOTCELL; // Ty 03/22/98 - externalized
      break;

    case SPR_CELP:
      if (!P_GiveAmmo (player, am_cell,5))
        return;
      player->message = s_GOTCELLBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_SHEL:
      if (!P_GiveAmmo (player, am_shell,1))
        return;
      player->message = s_GOTSHELLS; // Ty 03/22/98 - externalized
      break;

    case SPR_SBOX:
      if (!P_GiveAmmo (player, am_shell,5))
        return;
      player->message = s_GOTSHELLBOX; // Ty 03/22/98 - externalized
      break;

    case SPR_BPAK:
      if (!player->backpack)
        {
          for (i=0 ; i<NUMAMMO ; i++)
            player->maxammo[i] *= 2;
          player->backpack = true;
        }
      for (i=0 ; i<NUMAMMO ; i++)
        P_GiveAmmo (player, i, 1);
      player->message = s_GOTBACKPACK; // Ty 03/22/98 - externalized
      break;

        // weapons
    case SPR_BFUG:
      if (!P_GiveWeapon (player, wp_bfg, false) )
        return;
      player->message = s_GOTBFG9000; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_MGUN:
      if (!P_GiveWeapon (player, wp_chaingun, (special->flags&MF_DROPPED)!=0) )
        return;
      player->message = s_GOTCHAINGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_CSAW:
      if (!P_GiveWeapon (player, wp_chainsaw, false) )
        return;
      player->message = s_GOTCHAINSAW; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_LAUN:
      if (!P_GiveWeapon (player, wp_missile, false) )
        return;
      player->message = s_GOTLAUNCHER; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_PLAS:
      if (!P_GiveWeapon (player, wp_plasma, false) )
        return;
      player->message = s_GOTPLASMA; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_SHOT:
      if (!P_GiveWeapon (player, wp_shotgun, (special->flags&MF_DROPPED)!=0 ) )
        return;
      player->message = s_GOTSHOTGUN; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    case SPR_SGN2:
      if (!P_GiveWeapon(player, wp_supershotgun, (special->flags&MF_DROPPED)!=0))
        return;
      player->message = s_GOTSHOTGUN2; // Ty 03/22/98 - externalized
      sound = sfx_wpnup;
      break;

    default:
      I_Error ("P_SpecialThing: Unknown gettable thing");
    }

  if (special->flags & MF_COUNTITEM)
    player->itemcount++;
  P_RemoveMobj (special);
  player->bonuscount += BONUSADD;

  /* cph 20028/10 - for old-school DM addicts, allow old behavior
   * where only consoleplayer's pickup sounds are heard */
  // displayplayer, not consoleplayer, for viewing multiplayer demos
  if (!comp[comp_sound] || player == &players[displayplayer])
    S_StartSound (player->mo, sound | PICKUP_SOUND);   // killough 4/25/98
}

//
// KillMobj
//
// killough 11/98: make static
static void P_KillMobj(mobj_t *source, mobj_t *target)
{
  mobjtype_t item;
  mobj_t     *mo;

  target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

  if (target->type != MT_SKULL)
    target->flags &= ~MF_NOGRAVITY;

  target->flags |= MF_CORPSE|MF_DROPOFF;
  target->height >>= 2;

  if (!((target->flags ^ MF_COUNTKILL) & (MF_FRIEND | MF_COUNTKILL)))
    totallive--;

  if (source && source->player)
    {
      // count for intermission
      if (target->flags & MF_COUNTKILL)
        source->player->killcount++;
      if (target->player)
        source->player->frags[target->player-players]++;
    }
    else
      if (target->flags & MF_COUNTKILL) { /* Add to kills tally */
  if ((compatibility_level < lxdoom_1_compatibility) || !netgame) {
    if (!netgame)
      // count all monster deaths,
      // even those caused by other monsters
      players[0].killcount++;
  } else
    if (!deathmatch) {
      // try and find a player to give the kill to, otherwise give the
      // kill to a random player.  this fixes the missing monsters bug
      // in coop - rain
      // CPhipps - not a bug as such, but certainly an inconsistency.
      if (target->lastenemy && target->lastenemy->health > 0
    && target->lastenemy->player) // Fighting a player
          target->lastenemy->player->killcount++;
        else {
        // cph - randomely choose a player in the game to be credited
        //  and do it uniformly between the active players
        unsigned int activeplayers = 0, player, i;

        for (player = 0; player<MAXPLAYERS; player++)
    if (playeringame[player])
      activeplayers++;

        if (activeplayers) {
    player = P_Random(pr_friends) % activeplayers;

    for (i=0; i<MAXPLAYERS; i++)
      if (playeringame[i])
        if (!player--)
          players[i].killcount++;
        }
      }
    }
      }

  if (target->player)
    {
      // count environment kills against you
      if (!source)
        target->player->frags[target->player-players]++;

      target->flags &= ~MF_SOLID;
      target->player->playerstate = PST_DEAD;
      P_DropWeapon (target->player);

      if (target->player == &players[consoleplayer] && (automapmode & am_active))
        AM_Stop();    // don't die in auto map; switch view prior to dying
    }

  if (target->health < -target->info->spawnhealth && target->info->xdeathstate)
    P_SetMobjState (target, target->info->xdeathstate);
  else
    P_SetMobjState (target, target->info->deathstate);

  target->tics -= P_Random(pr_killtics)&3;

  if (target->tics < 1)
    target->tics = 1;

  // Drop stuff.
  // This determines the kind of object spawned
  // during the death frame of a thing.

  switch (target->type)
    {
    case MT_WOLFSS:
    case MT_POSSESSED:
      item = MT_CLIP;
      break;

    case MT_SHOTGUY:
      item = MT_SHOTGUN;
      break;

    case MT_CHAINGUY:
      item = MT_CHAINGUN;
      break;

    default:
      return;
    }

  mo = P_SpawnMobj (target->x,target->y,ONFLOORZ, item);
  mo->flags |= MF_DROPPED;    // special versions of items
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//

void P_DamageMobj(mobj_t *target,mobj_t *inflictor, mobj_t *source, int damage)
{
  player_t *player;
  boolean justhit = false;          /* killough 11/98 */

  /* killough 8/31/98: allow bouncers to take damage */
  if (!(target->flags & (MF_SHOOTABLE | MF_BOUNCES)))
    return; // shouldn't happen...

  if (target->health <= 0)
    return;

  if (target->flags & MF_SKULLFLY)
    target->momx = target->momy = target->momz = 0;

  player = target->player;
  if (player && gameskill == sk_baby)
    damage >>= 1;   // take half damage in trainer mode

  // Some close combat weapons should not
  // inflict thrust and push the victim out of reach,
  // thus kick away unless using the chainsaw.

  if (inflictor && !(target->flags & MF_NOCLIP) &&
      (!source || !source->player ||
       source->player->readyweapon != wp_chainsaw))
    {
      unsigned ang = R_PointToAngle2 (inflictor->x, inflictor->y,
                                      target->x,    target->y);

      fixed_t thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

      // make fall forwards sometimes
      if ( damage < 40 && damage > target->health
           && target->z - inflictor->z > 64*FRACUNIT
           && P_Random(pr_damagemobj) & 1)
        {
          ang += ANG180;
          thrust *= 4;
        }

      ang >>= ANGLETOFINESHIFT;
      target->momx += FixedMul (thrust, finecosine[ang]);
      target->momy += FixedMul (thrust, finesine[ang]);

      /* killough 11/98: thrust objects hanging off ledges */
      if (target->intflags & MIF_FALLING && target->gear >= MAXGEAR)
        target->gear = 0;
    }

  // player specific
  if (player)
    {
      // end of game hell hack
      if (target->subsector->sector->special == 11 && damage >= target->health)
        damage = target->health - 1;

      // Below certain threshold,
      // ignore damage in GOD mode, or with INVUL power.
      // killough 3/26/98: make god mode 100% god mode in non-compat mode

      if ((damage < 1000 || (!comp[comp_god] && (player->cheats&CF_GODMODE))) &&
          (player->cheats&CF_GODMODE || player->powers[pw_invulnerability]))
        return;

      if (player->armortype)
        {
          int saved = player->armortype == 1 ? damage/3 : damage/2;
          if (player->armorpoints <= saved)
            {
              // armor is used up
              saved = player->armorpoints;
              player->armortype = 0;
            }
          player->armorpoints -= saved;
          damage -= saved;
        }

      player->health -= damage;       // mirror mobj health here for Dave
      if (player->health < 0)
        player->health = 0;

      player->attacker = source;
      player->damagecount += damage;  // add damage after armor / invuln
#ifdef IPHONE
		if ( player == &players[consoleplayer] && !demoplayback ) {	// vibe during demos is annoying
            // JDS: Stop vibration on pain
            // SysIPhoneVibrate();
		}
#endif
      if (player->damagecount > 100)
        player->damagecount = 100;  // teleport stomp does 10k points...
    }

  // do the damage
  target->health -= damage;
  if (target->health <= 0)
    {
      P_KillMobj (source, target);
      return;
    }

  // killough 9/7/98: keep track of targets so that friends can help friends
  if (mbf_features)
    {
      /* If target is a player, set player's target to source,
       * so that a friend can tell who's hurting a player
       */
      if (player)
  P_SetTarget(&target->target, source);

      /* killough 9/8/98:
       * If target's health is less than 50%, move it to the front of its list.
       * This will slightly increase the chances that enemies will choose to
       * "finish it off", but its main purpose is to alert friends of danger.
       */
      if (target->health*2 < target->info->spawnhealth)
  {
    thinker_t *cap = &thinkerclasscap[target->flags & MF_FRIEND ?
             th_friends : th_enemies];
    (target->thinker.cprev->cnext = target->thinker.cnext)->cprev =
      target->thinker.cprev;
    (target->thinker.cnext = cap->cnext)->cprev = &target->thinker;
    (target->thinker.cprev = cap)->cnext = &target->thinker;
  }
    }

  if (P_Random (pr_painchance) < target->info->painchance &&
      !(target->flags & MF_SKULLFLY)) { //killough 11/98: see below
    if (mbf_features)
      justhit = true;
    else
      target->flags |= MF_JUSTHIT;    // fight back!

    P_SetMobjState(target, target->info->painstate);
  }

  target->reactiontime = 0;           // we're awake now...

  /* killough 9/9/98: cleaned up, made more consistent: */

  if (source && source != target && source->type != MT_VILE &&
      (!target->threshold || target->type == MT_VILE) &&
      ((source->flags ^ target->flags) & MF_FRIEND ||
       monster_infighting ||
       !mbf_features))
    {
      /* if not intent on another player, chase after this one
       *
       * killough 2/15/98: remember last enemy, to prevent
       * sleeping early; 2/21/98: Place priority on players
       * killough 9/9/98: cleaned up, made more consistent:
       */

      if (!target->lastenemy || target->lastenemy->health <= 0 ||
    (!mbf_features ?
     !target->lastenemy->player :
     !((target->flags ^ target->lastenemy->flags) & MF_FRIEND) &&
     target->target != source)) // remember last enemy - killough
  P_SetTarget(&target->lastenemy, target->target);

      P_SetTarget(&target->target, source);       // killough 11/98
      target->threshold = BASETHRESHOLD;
      if (target->state == &states[target->info->spawnstate]
          && target->info->seestate != S_NULL)
        P_SetMobjState (target, target->info->seestate);
    }

  /* killough 11/98: Don't attack a friend, unless hit by that friend.
   * cph 2006/04/01 - implicitly this is only if mbf_features */
  if (justhit && (target->target == source || !target->target ||
      !(target->flags & target->target->flags & MF_FRIEND)))
    target->flags |= MF_JUSTHIT;    // fight back!
}
