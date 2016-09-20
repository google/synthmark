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

#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController ()

@end

@implementation ViewController
@synthesize m_appObject;
@synthesize buttonTest1;
@synthesize buttonTest2;
@synthesize buttonTest3;
@synthesize textViewOutput;
@synthesize activityIndicatorRunning;
@synthesize labelShortStatus;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    AppDelegate *appDelegate = (AppDelegate *) [UIApplication sharedApplication].delegate;
    m_appObject = appDelegate.m_appObject;
}

-(void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(testStarted:)
                                                 name:APP_NOTIFICATION_TEST_STARTED
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(testUpdated:)
                                                 name:APP_NOTIFICATION_TEST_UPDATE
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(testCompleted:)
                                                 name:APP_NOTIFICATION_TEST_COMPLETED
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(testShortUpdate:)
                                                 name:APP_NOTIFICATION_TEST_SHORT_UPDATE
                                               object:nil];
}

-(void) viewWillDisappear:(BOOL)animated {

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:APP_NOTIFICATION_TEST_STARTED
                                                  object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:APP_NOTIFICATION_TEST_UPDATE
                                                  object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:APP_NOTIFICATION_TEST_COMPLETED
                                                  object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:APP_NOTIFICATION_TEST_SHORT_UPDATE
                                                  object:nil];

    [super viewWillDisappear:animated];
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
  // Dispose of any resources that can be recreated.
}

-(IBAction) buttonPressed:(id)sender {
  if (sender == buttonTest1) {
      [m_appObject startTest:1];
  } else if (sender == buttonTest2) {
      [m_appObject startTest:2];
  } else if (sender == buttonTest3) {
      [m_appObject startTest:3];
  }
}

#pragma mark - notifications
-(void) testStarted:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];

    NSString *str = [[NSString alloc]initWithFormat:@"Starting test %d\n", testId];
    [textViewOutput setText:str];
//    [activityIndicatorRunning setHidden:false];
    [activityIndicatorRunning startAnimating];
}


-(void) testUpdated:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];
    NSString *message = [dict objectForKey:APP_NOTIFICATION_KEY_MESSAGE];

    NSString *str = [[NSString alloc]initWithFormat:@"[%d] %@", testId, message];
    [textViewOutput setText:[textViewOutput.text stringByAppendingString:str]];
}

-(void) testCompleted:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];
    NSString *str = [[NSString alloc]initWithFormat:@"Finished test %d\n", testId];
    [textViewOutput setText:[textViewOutput.text stringByAppendingString:str]];

//    [activityIndicatorRunning setHidden:true];
    [activityIndicatorRunning stopAnimating];
}

-(void) testShortUpdate:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];
    NSString *message = [dict objectForKey:APP_NOTIFICATION_KEY_MESSAGE];

    NSString *str = [[NSString alloc]initWithFormat:@"%@", message];
    [labelShortStatus setText:str];
}

@end
