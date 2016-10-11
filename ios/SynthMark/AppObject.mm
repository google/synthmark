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

#import "AppObject.h"
#include <stdio.h>
#include <sstream>
#include <math.h>
#include "SynthMark.h"
#include "tools/NativeTest.h"
#include "tools/TimingAnalyzer.h"
#include "synth/Synthesizer.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"
#include "tools/LatencyMarkHarness.h"


#include "synth/IncludeMeOnce.h"

@interface AppObject () {

}
@property (nonatomic) NativeTest *pNativeTest;


@end

@implementation AppObject
@synthesize mTestRunning;
@synthesize mCurrentTest;

-(id) init {
    self = [super init];
    if (self) {
        //init any relevant data
        mTestRunning = false;
        self.pNativeTest = new NativeTest();

        if (self.pNativeTest->getTestCount() > 0) {
            mCurrentTest = 0;
        } else {
            mCurrentTest = -1;
        }
    }
    return self;
}

-(void) dealloc {
    delete self.pNativeTest;
}

-(BOOL) isTestRunning {
    return mTestRunning;
}


-(void) startTest:(int) testId {
    if (mTestRunning) {
        NSString * msg = @"Can't start test when another test is running\n";
        [self postNotification_TestUpdate:testId message:msg];
        NSLog(@"%@",msg);
        return;
    }

    if (self.pNativeTest != NULL) {
        NSLog(@"start test...");

        mTestRunning = true;

        //test task
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [self postNotification_TestStarted:testId];
            [self postNotification_TestUpdate:testId message:@"Starting...\n"];

            self.pNativeTest->init(testId);

            self.pNativeTest->run();

            std::string result = self.pNativeTest->getResult();
            self.pNativeTest->closeTest();

            NSString *resultMessage = [NSString stringWithUTF8String:result.c_str()];
            mTestRunning = false;

            [self postNotification_TestUpdate:testId message:resultMessage];
            [self postNotification_TestUpdate:testId message:@"Test complete.\n"];
            [self postNotification_TestCompleted:testId];
        });

        //test progress task
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

            [NSThread sleepForTimeInterval:0.3f];

            while (mTestRunning) {
                int progress = self.pNativeTest->getProgress();
                int status = self.pNativeTest->getStatus();
                NSString *message = [[NSString alloc] initWithFormat:@"%d",progress];

                [self postNotification_TestShortUpdate:testId message:message];
                if (status != 2 /*running*/) {
                    break;
                }

                [NSThread sleepForTimeInterval:PROGRESS_REFRESH_RATE_SEC];
            }
        });
    }
}

-(NativeTest*) getNativeTest {
    return self.pNativeTest;
}

-(int) getCurrentTestId {
    return mCurrentTest;
}

-(void) setCurrentTest:(int) currentTest {
    mCurrentTest = currentTest;
}

#pragma mark - notifications

-(void) postNotification_TestStarted:(int) testId {

    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
                           [NSNumber numberWithInt:testId], APP_NOTIFICATION_KEY_TEST_ID,
                           nil];

    NSNotification * note = [NSNotification notificationWithName:APP_NOTIFICATION_TEST_STARTED
                                                          object:self
                                                        userInfo:dict];

    [[NSNotificationCenter defaultCenter] performSelectorOnMainThread:@selector(postNotification:)
                                                           withObject:note waitUntilDone:NO];
}

-(void) postNotification_TestUpdate:(int) testId message:(NSString*) message {

    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
                           [NSNumber numberWithInt:testId], APP_NOTIFICATION_KEY_TEST_ID,
                           message, APP_NOTIFICATION_KEY_MESSAGE,
                           nil];

    NSNotification * note = [NSNotification notificationWithName:APP_NOTIFICATION_TEST_UPDATE
                                                          object:self
                                                        userInfo:dict];

    [[NSNotificationCenter defaultCenter] performSelectorOnMainThread:@selector(postNotification:)
                                                           withObject:note waitUntilDone:NO];

}

-(void) postNotification_TestCompleted:(int) testId {
    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
                           [NSNumber numberWithInt:testId], APP_NOTIFICATION_KEY_TEST_ID,
                           nil];

    NSNotification * note = [NSNotification notificationWithName:APP_NOTIFICATION_TEST_COMPLETED
                                                          object:self
                                                        userInfo:dict];

    [[NSNotificationCenter defaultCenter] performSelectorOnMainThread:@selector(postNotification:)
                                                           withObject:note waitUntilDone:NO];
}

-(void) postNotification_TestShortUpdate:(int) testId message:(NSString*) message {

    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:
                           [NSNumber numberWithInt:testId], APP_NOTIFICATION_KEY_TEST_ID,
                           message, APP_NOTIFICATION_KEY_MESSAGE,
                           nil];

    NSNotification * note = [NSNotification notificationWithName:APP_NOTIFICATION_TEST_SHORT_UPDATE
                                                          object:self
                                                        userInfo:dict];

    [[NSNotificationCenter defaultCenter] performSelectorOnMainThread:@selector(postNotification:)
                                                           withObject:note waitUntilDone:NO];

}


@end
