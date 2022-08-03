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

#import "EpisodeMenuViewController.h"
#include "doomiphone.h"
#include "iphone_delegate.h"
#import "MissionMenuViewController.h"

#if GAME_DOOM

#define TOTAL_EPISODES 4

static const char * const EpisodeNames[TOTAL_EPISODES][4] = {
    { "Episode 1", "Knee-Deep in the Dead", "Episode1Background", "Episode1Background_Highlighted" },
    { "Episode 2", "The Shores of Hell", "Episode2Background", "Episode2Background_Highlighted" },
    { "Episode 3", "Inferno", "Episode3Background", "Episode3Background_Highlighted" },
    { "Episode 4", "Thy Flesh Consumed", "Episode4Background", "Episode4Background_Highlighted" }
};

#endif

#if GAME_FINALDOOM

#define TOTAL_EPISODES 2

static const char * const EpisodeNames[TOTAL_EPISODES][4] = {
    { "", "TNT: Evilution", "Episode1Background", "Episode1Background_Highlighted" },
    { "", "The Plutonia Experiment", "Episode2Background", "Episode2Background_Highlighted" }
};

#endif


@interface Doom_EpisodeMenuViewController ()

@property (nonatomic, retain) UITableView *episodeList;

@end


/*
 ================================================================================================
 EpisodeMenuViewController
 
 ================================================================================================
 */
@implementation Doom_EpisodeMenuViewController

@synthesize episodeList;
@synthesize episodeCell;

/*
 ========================
 Doom_EpisodeMenuViewController::initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
            
        episodeSelection = -1;
        [ nextButton setEnabled: NO ];
        [ nextLabel setEnabled: NO ];
        
    }
    return self;
}

/*
 ========================
 Doom_EpisodeMenuViewController::didReceiveMemoryWarning
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
 Doom_EpisodeMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
	
	[ nextButton setEnabled: NO ];
	[ nextLabel setEnabled: NO ];
    
    // TODO: Select the current episode. Haven't yet figured out a good way to get the table
    // view to load with a default row selected, so for now always select episode 1.
#if !TARGET_OS_TV
    int initialEpisode = 0;
    NSIndexPath *initialPath = [NSIndexPath indexPathForRow:initialEpisode inSection:0];
    
    [self.episodeList selectRowAtIndexPath:initialPath animated:YES scrollPosition:UITableViewScrollPositionNone];
    [self handleSelectionAtIndexPath:initialPath];
    self.episodeList.separatorStyle = UITableViewCellSeparatorStyleNone;
#endif
}

/*
 ========================
 Doom_EpisodeMenuViewController::BackToMain
 ========================
 */
- (IBAction) BackToMain {
    
    [self.navigationController popViewControllerAnimated:NO];
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

/*
 ========================
 Doom_EpisodeMenuViewController::NextToMissions
 ========================
 */
- (IBAction) NextToMissions {
    
    Doom_MissionMenuViewController *vc = [[Doom_MissionMenuViewController alloc] initWithNibName:[gAppDelegate GetNibNameForDevice:@"MissionMenuView"] bundle:nil];
    
#if GAME_FINALDOOM
    
    if (episodeSelection == 1) {
        iphoneIWADSelect("plutonia.wad");
    } else {
        iphoneIWADSelect("tnt.wad");
    }
    
    char full_iwad[1024];
    
    I_FindFile( doom_iwad, ".wad", full_iwad );
    
    // fall back to default DOOM wad
    if( full_iwad[0] == '\0' ) {
        I_FindFile( "doom.wad", ".wad", full_iwad );
        if( doom_iwad ) free(doom_iwad);
        doom_iwad = strdup(full_iwad);
    } else if( strcmp(doom_iwad,full_iwad) != 0 ) {
        if( doom_iwad ) free(doom_iwad);
        doom_iwad = strdup( full_iwad );
    }
    
    iphoneDoomStartup();
    
#endif

    [self.navigationController pushViewController:vc animated:NO];
    [vc setEpisode:episodeSelection ];
    [vc release];
    
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

/*
 ========================
 
 UITableView interface
 
 ========================
 */

- (void)handleSelectionAtIndexPath:(NSIndexPath*)indexPath {
    
    Cvar_SetValue( episode->name, indexPath.row );
    
    [self setCellSelected:YES atIndexPath:indexPath];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return TOTAL_EPISODES;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *MyIdentifier = @"EpisodeIdentifier";
    UITableViewCell *cell = (UITableViewCell*)[self.episodeList dequeueReusableCellWithIdentifier:MyIdentifier];
    
    if (cell == nil) {
        //cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier] autorelease];
        [[NSBundle mainBundle] loadNibNamed:[gAppDelegate GetNibNameForDevice:@"EpisodeCell"] owner:self options:nil];
        
        if ( episodeCell == nil ) {
            // Couldn't create from nib file, load a default cell.
            cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier] autorelease];
        } else {
            cell = episodeCell;
            self.episodeCell = nil;
            
            // Save the good label size here before it gets modified by the code below.
//            UILabel *episodeNameLabel;
//            episodeNameLabel = (UILabel *)[cell viewWithTag:2];
//
//            maximumNameLabelFrame = episodeNameLabel.frame;
        }
    }
    
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    
    BOOL isCellSelected = ( episode->value == indexPath.row)? YES: NO;
    
    [self setCellSelected:isCellSelected cell:cell];
    
    UILabel *episodeNumberLabel;
    episodeNumberLabel = (UILabel *)[cell viewWithTag:1];
    episodeNumberLabel.text = [NSString stringWithCString:EpisodeNames[indexPath.row][0] encoding:NSASCIIStringEncoding];
    
    UILabel *episodeNameLabel;
    episodeNameLabel = (UILabel *)[cell viewWithTag:2];
    NSString* episodeNameText = [NSString stringWithCString:EpisodeNames[indexPath.row][1] encoding:NSASCIIStringEncoding];
    
//    CGSize expectedLabelSize = [episodeNameText boundingRectWithSize:maximumNameLabelFrame.size
//                                                             options:NSStringDrawingUsesLineFragmentOrigin
//                                                          attributes:@{NSFontAttributeName: episodeNameLabel.font}
//                                                             context:nil].size;
//
//    //adjust the label the the new height.
//    CGRect newFrame = maximumNameLabelFrame;
//    newFrame.size.height = expectedLabelSize.height;
//    episodeNameLabel.frame = newFrame;
    
    episodeNameLabel.text = episodeNameText;
    
    UIImageView* backgroundImage = (UIImageView *)[cell viewWithTag:3];
    
    backgroundImage.image = [UIImage imageNamed:[NSString stringWithCString:EpisodeNames[indexPath.row][2] encoding:NSASCIIStringEncoding]];
    backgroundImage.highlightedImage = [UIImage imageNamed:[NSString stringWithCString:EpisodeNames[indexPath.row][3] encoding:NSASCIIStringEncoding]];

    
    cell.backgroundColor = UIColor.clearColor;
    
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
        return 158;
    } else if (IS_TV) {
        return 185;
    } else {
        return 79;
    }
}

#if TARGET_OS_TV
- (void)didUpdateFocusInContext:(UIFocusUpdateContext *)context withAnimationCoordinator:(UIFocusAnimationCoordinator *)coordinator {
    
    [super didUpdateFocusInContext:context withAnimationCoordinator:coordinator];
    
    if ([context.nextFocusedView isKindOfClass:[UITableViewCell class]]) {
        [coordinator addCoordinatedAnimations:^{
            [context.nextFocusedView setBackgroundColor:[UIColor redColor]];
            [context.previouslyFocusedView setBackgroundColor:[UIColor clearColor]];
        } completion:nil];
    }
}
#endif

- (void)setCellSelected:(BOOL)selected atIndexPath:(NSIndexPath*)indexPath {
    // Get the cell that was selected.
    [ nextButton setEnabled: YES ];
    [ nextLabel setEnabled: YES ];
    episodeSelection = (int)indexPath.row;

#if TARGET_OS_TV
    [self NextToMissions];
#else
    UITableViewCell * cell = [episodeList cellForRowAtIndexPath:indexPath];
    [self setCellSelected:selected cell:cell];
#endif
}

- (void)setCellSelected:(BOOL)selected cell:(UITableViewCell*)cell {
    // Get the "selected" image
    UIImageView* selectionFrame = (UIImageView *)[cell viewWithTag:3];
    
    // Show the selected image
    selectionFrame.highlighted = selected;
}

@end
