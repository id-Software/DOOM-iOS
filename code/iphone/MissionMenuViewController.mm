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

#define TOTAL_EPISODES 4

static const char * const MissionNames[TOTAL_EPISODES][9] = {
    {"E1M1: Hanger", "E1M2: Nuclear Plant", "E1M3: Toxin Refinery", "E1M4: Command Control", "E1M5: Phobos Lab", "E1M6: Central Processing", "E1M7: Computer Station", "E1M8: Phobos Anomaly", "E1M9: Military Base"},
    {"E2M1: Deimos Anomaly", "E2M2: Containment Area", "E2M3: Refinery", "E2M4: Deimos Lab", "E2M5: Command Center", "E2M6: Halls of the Damned", "E2M7: Spawning Vats", "E2M8: Tower of Babel", "E2M9: Fortress of Mystery"},
    {"E3M1: Hell Keep ", "E3M2: Slough of Despair", "E3M3: Pandemonium", "E3M4: House of Pain", "E3M5: Unholy Cathedral", "E3M6: Mt. Erebus", "E3M7: Limbo", "E3M8: Dis", "E3M9: Warrens"},
    {"E4M1: Hell Beneath", "E4M2: Perfect Hatred", "E4M3: Sever The Wicked", "E4M4: Unruly Evil", "E4M5: They Will Repent", "E4M6: Against Thee Wickedly", "E4M7: And Hell Followed", "E4M8: Unto The Cruel", "E4M9: Fear"}
};

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
    localStartmap.episode = episodeSelected;
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
    episodeSelected = episode;
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
}

/*
 ========================
 
 UITableView interface
 
 ========================
 */

- (void)handleSelectionAtIndexPath:(NSIndexPath*)indexPath {
    
//    Cvar_SetValue( episode->name, indexPath.row );
    
    [self setCellSelected:YES atIndexPath:indexPath];
    
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 9;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *MyIdentifier = @"MyIdentifier";

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
        cell.textLabel.font = [UIFont fontWithName:@"idGinza Narrow" size:points * 2];
    } else {
        cell.textLabel.font = [UIFont fontWithName:@"idGinza Narrow" size:points];
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

@end
