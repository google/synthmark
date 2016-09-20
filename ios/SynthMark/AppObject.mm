//
//  AppObject.m
//  SynthMark
//
//  Created by Ricardo Garcia on 7/26/16.
//  Copyright © 2016 google. All rights reserved.
//

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

@interface AppObject ()
@property (nonatomic) NativeTest *pNativeTest;
@end

@implementation AppObject
@synthesize mTestRunning;

-(id) init {
    self = [super init];
    if (self) {
        //init any relevant data
        mTestRunning = false;
        self.pNativeTest = new NativeTest();
    }
    return self;
}

-(void) dealloc {
    delete self.pNativeTest;
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
