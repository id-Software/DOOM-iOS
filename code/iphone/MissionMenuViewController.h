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

#import <UIKit/UIKit.h>
#import "../ios/Label.h"
#import "../ios/LabelButton.h"
/*
 ================================================================================================
 Doom_MissionMenuViewController
 
 ================================================================================================
 */
@interface Doom_MissionMenuViewController : UIViewController <UITableViewDelegate, UITableViewDataSource>  {
    
    IBOutlet UITableView *missionList;

    IBOutlet UIImageView *   easySelection;
    IBOutlet UIImageView *   mediumSelection;
    IBOutlet UIImageView *   hardSelection;
    IBOutlet UIImageView *   NightmareSelection;
    
    IBOutlet idLabel *  easySelectionLabel;
    IBOutlet idLabel *  mediumSelectionLabel;
    IBOutlet idLabel *  hardSelectionLabel;
    IBOutlet idLabel *  nightmareSelectionLabel;
    
    IBOutlet idLabelButton * playButton;
    IBOutlet idLabel * playLabel;
    
    idLabelButton    * selectedMap;
    int episodeSelected;
    int mapSelected;
    int currentCell;
}

- (int)  getSkill;
- (void) playMap:(int)dataset
				:(int)episode
				:(int)map;

- (void) setEpisode: (int) episode;

-(IBAction)     BackPressed;
-(IBAction)     Play;

-(IBAction)     UpMission;
-(IBAction)     DownMission;

// Difficulty Setting
-(IBAction)     EasyPressed;
-(IBAction)     MediumPressed;
-(IBAction)     HardPressed;
-(IBAction)     NightmarePressed;

@end
