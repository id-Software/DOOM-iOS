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

#import "MissionMenuViewController.h"
#include "doomiphone.h"
#include "iphone_delegate.h"

@interface Doom_MissionMenuViewController ()

@property (nonatomic, retain) UITableView *missionList;

@end

/*
 ================================================================================================
 Doom_MissionMenuViewController
 
 ================================================================================================
 */
@implementation Doom_MissionMenuViewController

BOOL levelSelected = NO;

#if GAME_DOOM

#define TOTAL_EPISODES 4

static const char * const MissionNames[TOTAL_EPISODES][9] = {
    {"E1M1: Hanger", "E1M2: Nuclear Plant", "E1M3: Toxin Refinery", "E1M4: Command Control", "E1M5: Phobos Lab", "E1M6: Central Processing", "E1M7: Computer Station", "E1M8: Phobos Anomaly", "E1M9: Military Base"},
    {"E2M1: Deimos Anomaly", "E2M2: Containment Area", "E2M3: Refinery", "E2M4: Deimos Lab", "E2M5: Command Center", "E2M6: Halls of the Damned", "E2M7: Spawning Vats", "E2M8: Tower of Babel", "E2M9: Fortress of Mystery"},
    {"E3M1: Hell Keep ", "E3M2: Slough of Despair", "E3M3: Pandemonium", "E3M4: House of Pain", "E3M5: Unholy Cathedral", "E3M6: Mt. Erebus", "E3M7: Limbo", "E3M8: Dis", "E3M9: Warrens"},
    {"E4M1: Hell Beneath", "E4M2: Perfect Hatred", "E4M3: Sever The Wicked", "E4M4: Unruly Evil", "E4M5: They Will Repent", "E4M6: Against Thee Wickedly", "E4M7: And Hell Followed", "E4M8: Unto The Cruel", "E4M9: Fear"}
};
#endif

#if GAME_DOOM2

#define TOTAL_EPISODES 1

static const char * const MissionNames[TOTAL_EPISODES][32] = {
    {"MAP01: Entryway", "MAP02: Underhalls", "MAP03: The Gauntlet", "MAP04: The Focus", "MAP05: The Waste Tunnels", "MAP06: The Crusher", "MAP07: Dead Simple", "MAP08: Tricks and Traps", "MAP09: The Pit", "MAP10: Refueling Base", "MAP11: 'O' of Destruction!", "MAP12: The Factory", "MAP13: Downtown", "MAP14: The Inmost Dens", "MAP15: Industrial Zone", "MAP16: Suburbs", "MAP17: Tenements", "MAP18: The Courtyard", "MAP19: The Citadel", "MAP20: Gotcha!", "MAP21: Nirvana", "MAP22: The Catacombs", "MAP23: Barrels o' Fun", "MAP24: The Chasm", "MAP25: Bloodfalls", "MAP26: The Abandoned Mines", "MAP27: Monster Condo", "MAP28: The Spirit World", "MAP29: The Living End", "MAP30: Icon of Sin", "MAP31: Wolfenstein", "MAP32: Grosse"}
};
#endif


#if GAME_FINALDOOM

#define TOTAL_EPISODES 2

static const char * const MissionNames[TOTAL_EPISODES][32] = {
    {"MAP01: System Control","MAP02: Human BBQ","MAP03: Power Control","MAP04: Wormhole","MAP05: Hanger","MAP06: Open Season","MAP07: Prison","MAP08: Metal","MAP09: Stronghold","MAP10: Redemption","MAP11: Storage Facility","MAP12: Crater","MAP13: Nukage Processing","MAP14: Steel Works","MAP15: Dead Zone","MAP16: Deepest Reaches","MAP17: Processing Area","MAP18: Mill","MAP19: Shipping/Respawning","MAP20: Central Processing","MAP21: Administration Center","MAP22: Habitat","MAP23: Lunar Mining Project","MAP24: Quarry","MAP25: Baron's Den","MAP26: Ballistyx","MAP27: Mount Pain","MAP28: Heck","MAP29: River Styx","MAP30: Last Call","MAP31: Pharaoh","MAP32: Caribbean"},
    {"MAP01: Congo","MAP02: Well of Souls","MAP03: Aztec","MAP04: Caged","MAP05: Ghost Town","MAP06: Baron's Lair","MAP07: Caughtyard","MAP08: Realm","MAP09: Abattoire","MAP10: Onslaught","MAP11: Hunted","MAP12: Speed","MAP13: The Crypt","MAP14: Genesis","MAP15: The Twilight (Exit to secret level)","MAP16: The Omen","MAP17: Compound","MAP18: Neurosphere","MAP19: NME","MAP20: The Death Domain","MAP21: Slayer","MAP22: Impossible Mission","MAP23: Tombstone","MAP24: The Final Frontier","MAP25: The Temple of Darkness","MAP26: Bunker","MAP27: Anti-Christ","MAP28: The Sewers","MAP29: Odyssey of Noises","MAP30: The Gateway of Hell","MAP31: Cyberden (Exit to super secret level)","MAP32: Go 2 It"}
};
#endif

#if GAME_SIGIL

#define TOTAL_EPISODES 5

// Just going along with the five-episode concept for now

static const char * const MissionNames[TOTAL_EPISODES][9] = {
    {"E1M1: Hanger", "E1M2: Nuclear Plant", "E1M3: Toxin Refinery", "E1M4: Command Control", "E1M5: Phobos Lab", "E1M6: Central Processing", "E1M7: Computer Station", "E1M8: Phobos Anomaly", "E1M9: Military Base"},
    {"E2M1: Deimos Anomaly", "E2M2: Containment Area", "E2M3: Refinery", "E2M4: Deimos Lab", "E2M5: Command Center", "E2M6: Halls of the Damned", "E2M7: Spawning Vats", "E2M8: Tower of Babel", "E2M9: Fortress of Mystery"},
    {"E3M1: Hell Keep ", "E3M2: Slough of Despair", "E3M3: Pandemonium", "E3M4: House of Pain", "E3M5: Unholy Cathedral", "E3M6: Mt. Erebus", "E3M7: Limbo", "E3M8: Dis", "E3M9: Warrens"},
    {"E4M1: Hell Beneath", "E4M2: Perfect Hatred", "E4M3: Sever The Wicked", "E4M4: Unruly Evil", "E4M5: They Will Repent", "E4M6: Against Thee Wickedly", "E4M7: And Hell Followed", "E4M8: Unto The Cruel", "E4M9: Fear"},
    {"Baphomet's Demesne", "Sheol", "Cages of the Damned", "Paths of Wretchedness", "Abaddon's Void", "Unspeakable Persecution", "Nightmare Underworld", "Halls of Perdition", "Realm of Iblis"}
};
#endif

//{"MAP01: System Control","MAP02: Human BBQ","MAP03: Power Control","MAP04: Wormhole","MAP05: Hanger","MAP06: Open Season","MAP07: Prison","MAP08: Metal","MAP09: Stronghold","MAP10: Redemption","MAP11: Storage Facility","MAP12: Crater","MAP13: Nukage Processing","MAP14: Steel Works","MAP15: Dead Zone","MAP16: Deepest Reaches","MAP17: Processing Area","MAP18: Mill","MAP19: Shipping/Respawning","MAP20: Central Processing","MAP21: Administration Center","MAP22: Habitat","MAP23: Lunar Mining Project","MAP24: Quarry","MAP25: Baron's Den","MAP26: Ballistyx","MAP27: Mount Pain","MAP28: Heck","MAP29: River Styx","MAP30: Last Call","MAP31: Pharaoh","MAP32: Caribbean"}



@synthesize missionList;

/*
 ========================
 Doom_MissionMenuViewController::initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

/*
 ========================
 Doom_MissionMenuViewController::didReceiveMemoryWarning
 ========================
 */
- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
 ========================
 Doom_MissionMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [ playButton setEnabled: NO ];
    [ playLabel setEnabled: NO ];
    
    selectedMap = nil;
    mapSelected = -1;
    currentCell = 0;
    
    easySelection.hidden        = NO;
    easySelectionLabel.hidden   = NO;
    mediumSelection.hidden      = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelection.hidden        = YES;
    hardSelectionLabel.hidden   = YES;
    NightmareSelection.hidden   = YES;
    nightmareSelectionLabel.hidden = YES;
    
    [missionList reloadData];
}

/*
 ========================
 Doom_MissionMenuViewController::setEpisode
 ========================
 */
- (void) setEpisode: (int) episode {
    
    episodeSelected = episode;
}

/*
 ========================
 Doom_MissionMenuViewController::BackPressed
 ========================
 */
-(IBAction)     BackPressed {
    
    [self.navigationController popViewControllerAnimated:NO];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
    
}

/*
 ========================
 Doom_MissionMenuViewController::getSkill
 ========================
 */
- (int)  getSkill {
    
    if( easySelection.hidden == NO ) {
        return 0;
    } else if( mediumSelection.hidden == NO ) {
        return 1;
    } else if( hardSelection.hidden == NO ) {
        return 2;
    } else if( NightmareSelection.hidden == NO ) {
        return 3;
    }
    
    return 0;
}

/*
 ========================
 Doom_MissionMenuViewController::UpMission
 ========================
 */
-(IBAction)     UpMission {
    currentCell -= 1;
    if (currentCell < 0) {
        currentCell = 0;
    }
    [missionList scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:currentCell inSection:0] atScrollPosition:UITableViewScrollPositionTop animated:YES];
}

/*
 ========================
 Doom_MissionMenuViewController::DownMission
 ========================
 */
-(IBAction)     DownMission {
    currentCell += 1;
    if (currentCell > 6) {
        currentCell = 6;
    }
    [missionList scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:currentCell inSection:0] atScrollPosition:UITableViewScrollPositionTop animated:YES];
}

/*
 ========================
 Doom_MissionMenuViewController::Play
 ========================
 */
-(IBAction)     Play {

    int skillLevel = [self getSkill];
    mapStart_t localStartmap;
    
    localStartmap.map = mapSelected;
    localStartmap.episode = episodeSelected + 1;
    localStartmap.dataset = 0;
    localStartmap.skill = skillLevel;
    
    StartSinglePlayerGame( localStartmap );
    
    [ gAppDelegate ShowGLView ];
}

/*
 ========================
 Doom_MissionMenuViewController::playMap
 ========================
 */
- (void) playMap:(int)dataset
				:(int)episode
				:(int) map {
    
	(void)dataset;
	
    [ playButton setEnabled: YES ];
    [ playLabel setEnabled: YES ];
    
    if( selectedMap != nil ) {
        [ selectedMap setEnabled: YES ];
    }
    //episodeSelected = episode;
    mapSelected = map;
    
    int mapTag = episode * 10 + ( map - 1 );
    selectedMap = (idLabelButton *)[ self.view viewWithTag: mapTag ];
    
    [selectedMap setEnabled: NO];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

/*
 ========================
 Doom_MissionMenuViewController::EasyPressed
 ========================
 */
-(IBAction)     EasyPressed {
    
    easySelection.hidden        = NO;
    easySelectionLabel.hidden   = NO;
    mediumSelection.hidden      = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelection.hidden        = YES;
    hardSelectionLabel.hidden   = YES;
    NightmareSelection.hidden   = YES;
    nightmareSelectionLabel.hidden = YES;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
#if TARGET_OS_TV
    [self Play];
#endif
}

/*
 ========================
 Doom_MissionMenuViewController::MediumPressed
 ========================
 */
-(IBAction)     MediumPressed {
    
    easySelection.hidden        = YES;
    mediumSelection.hidden      = NO;
    hardSelection.hidden        = YES;
    NightmareSelection.hidden   = YES;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = NO;
    hardSelectionLabel.hidden   = YES;
    nightmareSelectionLabel.hidden = YES;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
#if TARGET_OS_TV
    [self Play];
#endif

}

/*
 ========================
 Doom_MissionMenuViewController::HardPressed
 ========================
 */
-(IBAction)     HardPressed {
    
    easySelection.hidden        = YES;
    mediumSelection.hidden      = YES;
    hardSelection.hidden        = NO;
    NightmareSelection.hidden   = YES;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelectionLabel.hidden   = NO;
    nightmareSelectionLabel.hidden = YES;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
#if TARGET_OS_TV
    [self Play];
#endif

}

/*
 ========================
 Doom_MissionMenuViewController::NightmarePressed
 ========================
 */
-(IBAction)     NightmarePressed{
    
    easySelection.hidden        = YES;
    mediumSelection.hidden      = YES;
    hardSelection.hidden        = YES;
    NightmareSelection.hidden   = NO;
    
    easySelectionLabel.hidden   = YES;
    mediumSelectionLabel.hidden = YES;
    hardSelectionLabel.hidden   = YES;
    nightmareSelectionLabel.hidden = NO;
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
#if TARGET_OS_TV
    [self Play];
#endif
}

/*
 ========================
 
 UITableView interface
 
 ========================
 */

- (void)handleSelectionAtIndexPath:(NSIndexPath*)indexPath {
    
//    Cvar_SetValue( episode->name, indexPath.row );
    
    [self setCellSelected:YES atIndexPath:indexPath];
    
    
#if TARGET_OS_TV
    levelSelected = YES;
    [self setNeedsFocusUpdate];
    [self updateFocusIfNeeded];
#endif
    
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
#if GAME_DOOM
    return 9;
#else
    return 32;
#endif
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *MyIdentifier = @"MissionIdentifier";

    UITableViewCell *cell = (UITableViewCell*)[self.missionList dequeueReusableCellWithIdentifier:MyIdentifier];
    
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier] autorelease];
    }
    
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    
    cell.textLabel.text = [NSString stringWithCString:MissionNames[episodeSelected][indexPath.row] encoding:NSASCIIStringEncoding];
    
    cell.backgroundColor = UIColor.clearColor;
    
    cell.textLabel.textColor = [UIColor colorWithRed:185.0f/255.0f green:108.0f/255.0f blue:17.0f/255.0f alpha:1];
    cell.textLabel.highlightedTextColor = [UIColor colorWithRed:255.0f/255.0f green:204.0f/255.0f blue:0.0f/255.0f alpha:1];
    
    CGFloat points = cell.textLabel.font.pointSize;
    
    if (IS_IPAD) {
        cell.textLabel.font = [UIFont fontWithName:[gAppDelegate GetFontName] size:34];
    } else {
        cell.textLabel.font = [UIFont fontWithName:[gAppDelegate GetFontName] size:points];
    }

    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    
    [self handleSelectionAtIndexPath:indexPath];
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
    [self setCellSelected:NO atIndexPath:indexPath];
    
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (IS_IPAD) {
        return 90;
    } else {
        return 45;
    }
}

- (void)setCellSelected:(BOOL)selected atIndexPath:(NSIndexPath*)indexPath {
    // Get the cell that was selected.
    [ playButton setEnabled: YES ];
    [ playLabel setEnabled: YES ];
    mapSelected = (int)indexPath.row + 1;
    
    NSLog(@"episodeSelected %i mapSelected: %i", episodeSelected, mapSelected);
    
    [ self playMap: 0: episodeSelected+1: mapSelected ];

    UITableViewCell * cell = [missionList cellForRowAtIndexPath:indexPath];
    cell.textLabel.highlighted = selected;
}

#if TARGET_OS_TV
-(NSArray<id<UIFocusEnvironment>> *)preferredFocusEnvironments {
    if (levelSelected) {
        levelSelected = NO;
        return @[normalButton];
    }
    else {
        return @[missionList];
    }
}

- (void)didUpdateFocusInContext:(UIFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator {
    NSLog(@"%@", context.nextFocusedView);
    
    [super didUpdateFocusInContext:context withAnimationCoordinator:coordinator];
    
    if ([context.nextFocusedView isKindOfClass:[idLabelButton class]]) {
        
        if (context.nextFocusedView.tag > 0) {
            easySelection.hidden        = YES;
            easySelectionLabel.hidden   = YES;
            mediumSelection.hidden      = YES;
            mediumSelectionLabel.hidden = YES;
            hardSelection.hidden        = YES;
            hardSelectionLabel.hidden   = YES;
            NightmareSelection.hidden   = YES;
            nightmareSelectionLabel.hidden = YES;

            
            if (context.nextFocusedView.tag == 1) {
                easySelection.hidden        = NO;
                easySelectionLabel.hidden   = NO;
            } else if (context.nextFocusedView.tag == 2) {
                mediumSelection.hidden      = NO;
                mediumSelectionLabel.hidden = NO;
            } else if (context.nextFocusedView.tag == 3) {
                hardSelection.hidden        = NO;
                hardSelectionLabel.hidden   = NO;
            } else if (context.nextFocusedView.tag == 4) {
                NightmareSelection.hidden   = NO;
                nightmareSelectionLabel.hidden = NO;
            }
        }
    }
}
#endif

@end
