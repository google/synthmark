/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "SettingsViewController.h"

#include "tools/NativeTest.h"
#import "AppDelegate.h"
#import "ParamTableViewCell.h"

@interface SettingsViewController ()

@end

@implementation SettingsViewController
@synthesize m_appObject;
@synthesize buttonDone;
@synthesize buttonDefaults;
@synthesize labelTitle;
@synthesize tableViewParams;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.

    AppDelegate *appDelegate = (AppDelegate *) [UIApplication sharedApplication].delegate;
    m_appObject = appDelegate.m_appObject;
    

}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

    [self refreshView];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

-(void) refreshView {
    //get title

    NativeTest * pNativeTest = [m_appObject getNativeTest];
    int currentTestId = [m_appObject getCurrentTestId];

    if (pNativeTest != NULL) {

        NSString *testTitle = [[NSString alloc] initWithFormat:@"%s Settings",
                               pNativeTest->getTestName(currentTestId).c_str()];

        [labelTitle setText:testTitle];
    }
}

-(IBAction) buttonPressed:(id)sender {
    if (sender == buttonDone) {
        [self.navigationController popViewControllerAnimated:YES];
    } else if (sender == buttonDefaults) {
        NativeTest * pNativeTest = [m_appObject getNativeTest];
        if (pNativeTest != NULL) {

            int currentTestId = [m_appObject getCurrentTestId];

            ParamGroup * pGroup = pNativeTest->getParamGroup(currentTestId);

            if (pGroup != NULL) {

                int paramCount = pGroup->getParamCount();
                for (int i = 0; i < paramCount; i++) {

                    ParamBase * pBase = pGroup->getParamByIndex(i);
                    if (pBase != NULL) {
                        pBase->setDefaultValue();
                    }
                }
                [tableViewParams reloadData];
            }
        }
    }
}


#pragma mark -- table view

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    int rows = 0;
    NativeTest * pNativeTest = [m_appObject getNativeTest];
    if (pNativeTest != NULL) {
        int currentTestId = [m_appObject getCurrentTestId];
        rows = pNativeTest->getParamCount(currentTestId);
     }

    return rows;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)
    indexPath {
    NSInteger row = [indexPath row];
    NativeTest * pNativeTest = [m_appObject getNativeTest];
    if (pNativeTest == NULL) {
        return NULL;
    }
    int currentTestId = [m_appObject getCurrentTestId];

    ParamGroup *pGroup = pNativeTest->getParamGroup(currentTestId);
    if (pGroup == NULL) {
        return NULL;
    }

    ParamBase *pBase = pGroup->getParamByIndex(row);
    if (pBase == NULL) {
        return NULL;
    }

    //check if int or float for now.
    ParamTableViewCell * cell = (ParamTableViewCell*)[tableView dequeueReusableCellWithIdentifier:
        @"ParamTableViewCell"];

    if (cell == nil) {
        NSArray *nib = [[NSBundle mainBundle] loadNibNamed:@"ParamTableViewCell" owner:self
            options:nil];
        cell = [nib objectAtIndex:0];
    }

    //set the parameters!
    [cell setTestId:currentTestId paramId:row];

    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 70;
}





@end
