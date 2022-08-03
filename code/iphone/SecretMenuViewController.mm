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

#import "SecretMenuViewController.h"
#include "doomiphone.h"
#include "iphone_delegate.h"

@implementation Doom_SecretMenuViewController

/*
 ========================
 Doom_SecretMenuViewController::initWithNibName
 ========================
 */
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
                
    }
    return self;
}

/*
 ========================
 Doom_SecretMenuViewController::didReceiveMemoryWarning
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
 Doom_SecretMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self updateWadLabels];
    [self updatePwadList];
    
    //maximumSpeed =[[NSArray alloc] initWithObjects:@"Running",@"Crying",@"Boring",@"Working",nil];
    //warningTime = [[NSArray alloc] initWithObjects: @"Happy", @"Sad" , @"Good", @"joyce",nil];

    
    doomEpisodes = [[NSArray alloc] initWithObjects:@"E1", @"E2", @"E3", @"E4",nil ];
    doomLevels = [[NSArray alloc] initWithObjects:@"M1", @"M2", @"M3", @"M4", @"M5", @"M6", @"M7", @"M8", @"M9", nil ];
    
    doom2Levels = [[NSArray alloc] initWithObjects:
                    @"MAP01",
                    @"MAP02",
                    @"MAP03",
                    @"MAP04",
                    @"MAP05",
                    @"MAP06",
                    @"MAP07",
                    @"MAP08",
                    @"MAP09",
                    @"MAP10",
                    @"MAP11",
                    @"MAP12",
                    @"MAP13",
                    @"MAP14",
                    @"MAP15",
                    @"MAP16",
                    @"MAP17",
                    @"MAP18",
                    @"MAP19",
                    @"MAP20",
                    @"MAP21",
                    @"MAP22",
                    @"MAP23",
                    @"MAP24",
                    @"MAP25",
                    @"MAP26",
                    @"MAP27",
                    @"MAP28",
                    @"MAP29",
                    @"MAP30",
                    @"MAP31",
                    @"MAP32",
                   nil
                    ];
    
    skillLevels = [[NSArray alloc] initWithObjects:
                    @"Easy",
                    @"Medium",
                    @"Hard",
                    @"Nightmare!",
                    nil
                   ];
    
    
    
    self->skillPicker.dataSource = self;
    self->skillPicker.delegate = self;
    
    self->levelPicker.dataSource = self;
    self->levelPicker.delegate = self;

}

- (void)updateWadLabels {
    iwadLabel.text = [[NSString stringWithUTF8String:doom_iwad] lastPathComponent];
    pwadLabel.text = [[[NSString stringWithUTF8String:doom_pwads] lastPathComponent] stringByReplacingOccurrencesOfString:@":" withString:@""];
    
    [levelPicker reloadAllComponents];
    [levelPicker selectRow:0 inComponent:0 animated:NO];
}

- (void)updatePwadList {
    NSFileManager *filemgr = [NSFileManager defaultManager];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSArray *dirFiles = [filemgr contentsOfDirectoryAtPath:documentsDirectory error:nil];
    
    UIButton *button = NULL;
    int y = 5;
    for (id dir in dirFiles) {
        
        NSString *value = (NSString *)dir;
        
        if ([[value pathExtension] caseInsensitiveCompare:@"wad"]==NSOrderedSame){
            button = [UIButton buttonWithType:UIButtonTypeCustom];
            [button addTarget:self action:@selector(pwadButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
            [button setTitle:value forState:UIControlStateNormal];
            [button.titleLabel setFont:[UIFont fontWithName:@"Helvetica" size:16.0]];
            [button setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
            [button setTitleColor:[UIColor greenColor] forState:UIControlStateHighlighted];
            [button setTitleColor:[UIColor greenColor] forState:UIControlStateSelected];
            
            if( [[NSString stringWithUTF8String:doom_pwads] rangeOfString:dir options:NSCaseInsensitiveSearch].location != NSNotFound) {
                [button setSelected:(YES)];
            }
            button.frame = CGRectMake(15, y, 175, 22.0);
            
            [pwadScroller addSubview:button];
            y += 25;
        }
    }
    
    if( button ) {
        [pwadScroller setContentSize:CGSizeMake(
                                            pwadScroller.bounds.size.width,
                                            CGRectGetMaxY(button.frame)
                                            )];
    }
}

/*
 ========================
 Doom_SecretMenuViewController::BackToMain
 ========================
 */
- (IBAction) BackToMain {
    
    [self.navigationController popViewControllerAnimated:NO];
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

- (IBAction)loadDoomIwad:(id)sender {
    
    iphoneIWADSelect("doom.wad");
    [self updateWadLabels];

}

- (IBAction)loadDoom2Iwad:(id)sender {
    
    iphoneIWADSelect("doom2.wad");
    [self updateWadLabels];
    
}

- (IBAction)loadTNTIwad:(id)sender {
    
    iphoneIWADSelect("tnt.wad");
    [self updateWadLabels];
    
}

- (IBAction)loadPlutoniaIwad:(id)sender {
    
    iphoneIWADSelect("plutonia.wad");
    [self updateWadLabels];
    
}

- (IBAction)xmasPwadOn:(id)sender {
    
    iphonePWADAdd("spritx.wad");
    [self updateWadLabels];

}

- (IBAction)clearPWADs:(id)sender {
    
    iphoneClearPWADs();
    
    if ([[pwadScroller subviews] count] > 0) {
        for( UIView* subview in [pwadScroller subviews]) {
            if ([subview isKindOfClass:[UIButton class]]) {
                [(UIButton*)subview setSelected:NO];
            }
        }
    }
    [self updateWadLabels];

}

- (IBAction)pwadButtonPressed:(id)sender {
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    
    if( [(UIButton*)sender isSelected]) {
        NSString* full_pwad = [NSString pathWithComponents:[NSArray arrayWithObjects:documentsDirectory,[[(UIButton*)sender titleLabel] text], nil]];
        [(UIButton*)sender setSelected:NO];
        printf("PWAD: %s\n", [full_pwad UTF8String]);
        iphonePWADRemove([full_pwad UTF8String]);
    } else {
        NSString* full_pwad = [NSString pathWithComponents:[NSArray arrayWithObjects:documentsDirectory,[[(UIButton*)sender titleLabel] text], nil]];
        [(UIButton*)sender setSelected:YES];
        printf("PWAD: %s\n", [full_pwad UTF8String]);
        iphonePWADAdd([full_pwad UTF8String]);
    }
    [self updateWadLabels];
}

- (IBAction)playButtonPressed:(UIButton *)sender {
    
    mapStart_t localStartmap;

    if( [levelPicker numberOfComponents] == 2) {
        localStartmap.episode = (int) ([levelPicker selectedRowInComponent:0] + 1);
        localStartmap.map = (int) ([levelPicker selectedRowInComponent:1] + 1);
    } else {
        localStartmap.episode = 1;
        localStartmap.map = (int) ([levelPicker selectedRowInComponent:0] + 1);
    }
    
    localStartmap.dataset = 0;
    localStartmap.skill = (int) [skillPicker selectedRowInComponent:0];

    // load our selected IWAD and PWADs
    iphoneDoomStartup();
    StartSinglePlayerGame( localStartmap );
    
    [ gAppDelegate ShowGLView ];
    
}


- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
   
    if( pickerView == levelPicker && [ iwadLabel.text caseInsensitiveCompare:@"doom.wad"] == NSOrderedSame ) {
        return 2; // episode, map
    }
    
    return 1;
}

// The number of rows of data
- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
 
    if( pickerView == levelPicker ) {
        if( [ iwadLabel.text caseInsensitiveCompare:@"doom.wad"] == NSOrderedSame ) {
            if( component == 0 ) {
                return (NSInteger) doomEpisodes.count;
            } else if( component == 1 ) {
                return (NSInteger) doomLevels.count;
            }
        } else {
            return (NSInteger) doom2Levels.count;
        }
    }
    
    if( pickerView == skillPicker ) {
        return (NSInteger) skillLevels.count;
    }
    
    return 1;
}

// The data to return for the row and component (column) that's being passed in
- (NSString*)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
    if( pickerView == levelPicker ) {
        if( [ iwadLabel.text caseInsensitiveCompare:@"doom.wad"] == NSOrderedSame ) {
            if( component == 0 ) {
                return doomEpisodes[ (NSUInteger) row];
            } else if( component == 1 ) {
                return doomLevels[ (NSUInteger) row];
            }
        } else {
            return doom2Levels[ (NSUInteger) row];
        }
    }
    
    if( pickerView == skillPicker ) {
        return skillLevels[ (NSUInteger) row];
    }
    
    return nil;
}


- (void)dealloc {
    [pwadLabel release];
    [iwadLabel release];
    [levelPicker release];
    [skillPicker release];
    [super dealloc];
}
@end
