//
//  AppObject.h
//  SynthMark
//
//  Created by Ricardo Garcia on 7/26/16.
//  Copyright © 2016 google. All rights reserved.
//

#import <Foundation/Foundation.h>


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

-(void) postNotification_TestStarted:(int) testId;
-(void) postNotification_TestUpdate:(int) testId message:(NSString*) message;
-(void) postNotification_TestCompleted:(int) testId;
-(void) postNotification_TestShortUpdate:(int) testId message:(NSString*) message;

-(void) startTest:(int) testId;

@end
