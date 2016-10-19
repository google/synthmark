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

#include "tools/NativeTest.h"
#import "ViewController.h"
#import "AppDelegate.h"


@interface ViewController () {

}

@end

@implementation ViewController
@synthesize m_appObject;
@synthesize textViewOutput;
@synthesize activityIndicatorRunning;
@synthesize labelShortStatus;
@synthesize buttonPickTest;
@synthesize buttonTest;
@synthesize buttonSettings;

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.

    AppDelegate *appDelegate = (AppDelegate *) [UIApplication sharedApplication].delegate;
    m_appObject = appDelegate.m_appObject;

    NativeTest * pNativeTest = [m_appObject getNativeTest];
    int mCurrentTestId = [m_appObject getCurrentTestId];

    if (pNativeTest != NULL) {
        int testCount = pNativeTest->getTestCount();
        NSLog(@" test count %d, currentTest: %d", testCount, mCurrentTestId);
    }

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

    [self refreshSettings];
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
  if (sender == buttonPickTest) {
      NativeTest * pNativeTest = [m_appObject getNativeTest];
      int currentTestId = [m_appObject getCurrentTestId];

      if (pNativeTest != NULL) {
          int testCount = pNativeTest->getTestCount();
          NSLog(@" test count %d, currentTest: %d", testCount, currentTestId);

          NSString * title = @"Pick Test";
          //NSString * message = @"Select Test";

          UIAlertView *alert = [[UIAlertView alloc] initWithTitle:title message:NULL delegate:self
              cancelButtonTitle:@"cancel" otherButtonTitles:nil];

          alert.alertViewStyle = UIAlertViewStyleDefault;

          for (int i = 0; i < testCount; i++) {
              NSString * buttonText = [[NSString alloc] initWithFormat:@"%s",
                  pNativeTest->getTestName(i).c_str()];
              [alert addButtonWithTitle:buttonText];
          }
          alert.tag = 100;
          [alert show];

      }

  } else if (sender == buttonTest) {
      int currentTestId = [m_appObject getCurrentTestId];
      [m_appObject startTest:currentTestId];

  } else if (sender == buttonSettings) {
      [self performSegueWithIdentifier:@"showSettings" sender:self];
  }
}

-(void) refreshSettings {
    NSLog(@"refresh Settings");

    NativeTest * pNativeTest = [m_appObject getNativeTest];
    int currentTestId = [m_appObject getCurrentTestId];

    if (pNativeTest != NULL) {
        NSString * buttonText = @"Pick a test";

        if (currentTestId > -1) {
            buttonText = [[NSString alloc] initWithFormat:@"%s",
                pNativeTest->getTestName(currentTestId).c_str()];
        }
        [buttonPickTest setTitle:buttonText forState:UIControlStateNormal];
    }
}

#pragma mark - Alert View
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (alertView.tag == 100) {
        if (buttonIndex == 0) {
            //cancel
        } else {
            int testId = buttonIndex - 1;
            [m_appObject setCurrentTest:testId];
            [self refreshSettings];
        }
    }
}

-(void) setButtonsEnabled:(BOOL) enabled {
    [buttonSettings setEnabled:enabled];
    [buttonTest setEnabled:enabled];
    [buttonPickTest setEnabled:enabled];
}

#pragma mark - notifications
-(void) testStarted:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];

    NSString * testName = @"--";

    NativeTest * pNativeTest = [m_appObject getNativeTest];
    int currentTestId = [m_appObject getCurrentTestId];
    if (pNativeTest != NULL) {
        testName = [[NSString alloc] initWithFormat:@"%s",
            pNativeTest->getTestName(currentTestId).c_str()];
    }

    NSString *str = [[NSString alloc]initWithFormat:@"Starting test %@\n", testName];
    [textViewOutput setText:str];
    [activityIndicatorRunning startAnimating];
    [self setButtonsEnabled:false];
}


-(void) testUpdated:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];
    NSString *message = [dict objectForKey:APP_NOTIFICATION_KEY_MESSAGE];

    NSString *str = [[NSString alloc]initWithFormat:@"%@", message];
    [textViewOutput setText:[textViewOutput.text stringByAppendingString:str]];
}

-(void) testCompleted:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];

    NSString * testName = @"--";
    NativeTest * pNativeTest = [m_appObject getNativeTest];
    int currentTestId = [m_appObject getCurrentTestId];
    if (pNativeTest != NULL) {
        testName = [[NSString alloc] initWithFormat:@"%s",
            pNativeTest->getTestName(currentTestId).c_str()];
    }

    NSString *str = [[NSString alloc]initWithFormat:@"Finished test %@\n", testName];
    [textViewOutput setText:[textViewOutput.text stringByAppendingString:str]];
    [activityIndicatorRunning stopAnimating];
    [self setButtonsEnabled:true];
}

-(void) testShortUpdate:(NSNotification *) notification {
    NSDictionary * dict = [notification userInfo];
    int testId = [[dict objectForKey:APP_NOTIFICATION_KEY_TEST_ID] intValue];
    NSString *message = [dict objectForKey:APP_NOTIFICATION_KEY_MESSAGE];

    NSString *str = [[NSString alloc]initWithFormat:@"%@", message];
    [labelShortStatus setText:str];
}

@end
