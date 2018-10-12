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

#import "CreditsMenuViewController.h"
#include "doomiphone.h"
#include "iphone_delegate.h"

@interface Doom_CreditsMenuViewController ()

@property (nonatomic, retain) UITableView *creditsList;

@end

@implementation Doom_CreditsMenuViewController


@synthesize creditsList;

#define CREDITS_LINES 43

// 1 - heading 1
// 2 - heading 2
// 3 - normal text
// 4 - tiny text

static const char * const CreditText[CREDITS_LINES] = {
    "DOOM Classic",
    "",
    "Programming",
    "John Carmack",
    "Jeff Farrand",
    "Ryan Gerieve",
    "",
    "Art",
    "John Burnett",
    "Danny Keys",
    "",
    "Audio",
    "Zack Quarles",
    "",
    "QA Testing",
    "Sean Palomino",
    "",
    "Doom & Ultimate Doom originally created by",
    "id Software",
    "",
    "Programming",
    "John Carmack",
    "John Romero",
    "Dave Taylor",
    "",
    "Design",
    "John Romero",
    "Sandy Peterson",
    "",
    "Art",
    "Adrian Carmack",
    "Kevin Cloud",
    "",
    "Audio",
    "Bobby Prince",
    "",
    "Ultimate Doom",
    "additional Level Design",
    "John Anderson",
    "Shawn Green",
    "American McGee",
    "John Romero",
    "Tim Willts"
};

static const int CreditSizes[CREDITS_LINES] = {
    1,
    4,
    2,
    3,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    2,
    3,
    4,
    2,
    3,
    4,
    4,
    4,
    4,
    2,
    3,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    2,
    3,
    3,
    4,
    2,
    3,
    4,
    2,
    2,
    3,
    3,
    3,
    3,
    3,
};

/*
 ========================
 Doom_CreditsMenuViewController::initWithNibName
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
 Doom_CreditsMenuViewController::didReceiveMemoryWarning
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
 Doom_CreditsMenuViewController::viewDidLoad
 ========================
 */
- (void)viewDidLoad
{
    [super viewDidLoad];
    creditsList.allowsSelection = NO;
}

/*
 ========================
 Doom_CreditsMenuViewController::BackToMain
 ========================
 */
- (IBAction) BackToMain {
    
    [self.navigationController popViewControllerAnimated:NO];
    Sound_StartLocalSound( "iphone/controller_down_01_SILENCE.wav" );
}

/*
 ========================
 
 UITableView interface
 
 ========================
 */

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return CREDITS_LINES;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *MyIdentifier = @"CreditsIdentifier";
    
    UITableViewCell *cell = (UITableViewCell*)[self.creditsList dequeueReusableCellWithIdentifier:MyIdentifier];
    
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier] autorelease];
    }
    
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    
    cell.textLabel.text = [NSString stringWithCString:CreditText[indexPath.row] encoding:NSASCIIStringEncoding];
    
    cell.backgroundColor = UIColor.clearColor;
    
    cell.textLabel.textColor = [UIColor colorWithRed:255.0f/255.0f green:204.0f/255.0f blue:0.0f/255.0f alpha:1];
    
    CGFloat points = cell.textLabel.font.pointSize;
    
    switch (CreditSizes[indexPath.row]) {
        case 1:
            points = 24;
            break;
        case 2:
            points = 20;
            break;
        case 3:
            points = 16;
            break;
        case 4:
            points = 12;
            break;

        default:
            break;
    }
    
    if (IS_IPAD) {
        cell.textLabel.font = [UIFont fontWithName:[gAppDelegate GetFontName] size:points * 2];
    } else {
        cell.textLabel.font = [UIFont fontWithName:[gAppDelegate GetFontName] size:points];
    }
    
    cell.textLabel.textAlignment = NSTextAlignmentCenter;
    
    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    CGFloat size = 40;
    
    switch (CreditSizes[indexPath.row]) {
        case 1:
            size = 40;
            break;
        case 2:
            size = 30;
            break;
        case 3:
            size = 20;
            break;
        case 4:
            size = 10;
            break;
            
        default:
            break;
    }
    
    if (IS_IPAD) {
        return size * 2;
    } else {
        return size;
    }
}

#if TARGET_OS_TV
-(NSArray<id<UIFocusEnvironment>> *)preferredFocusEnvironments {
    return @[creditsList];
}
#endif

@end
