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

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <sstream>
#include <math.h>

class NativeTest;


#define APP_NOTIFICATION_TEST_STARTED   @"app_notification_test_started"
#define APP_NOTIFICATION_TEST_UPDATE    @"app_notification_test_update"
#define APP_NOTIFICATION_TEST_COMPLETED @"app_notification_test_completed"
#define APP_NOTIFICATION_TEST_SHORT_UPDATE @"app_notification_test_short_update"

#define APP_NOTIFICATION_KEY_TEST_ID    @"app_notification_key_test_id"
#define APP_NOTIFICATION_KEY_MESSAGE    @"app_notification_key_message"

#define PROGRESS_REFRESH_RATE_SEC 0.3
@interface AppObject : NSObject {
}

@property (atomic) BOOL mTestRunning;
@property (atomic) int mCurrentTest;

-(void) postNotification_TestStarted:(int) testId;
-(void) postNotification_TestUpdate:(int) testId message:(NSString*) message;
-(void) postNotification_TestCompleted:(int) testId;
-(void) postNotification_TestShortUpdate:(int) testId message:(NSString*) message;

-(void) startTest:(int) testId;

-(BOOL) isTestRunning;

-(NativeTest*) getNativeTest;
-(int) getCurrentTestId;
-(void) setCurrentTest:(int) currentTest;
@end
