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

#import <UIKit/UIKit.h>
#import "AppObject.h"

@interface ParamTableViewCell : UITableViewCell {

}
@property (weak, nonatomic) AppObject *m_appObject;
@property (atomic) int mTestId;
@property (atomic) int mParamId;

@property (weak, nonatomic) IBOutlet UILabel *labelDescription;
@property (weak, nonatomic) IBOutlet UILabel *labelValue;
@property (weak, nonatomic) IBOutlet UISlider *sliderValue;


-(void) setTestId:(int) testId paramId:(int) paramId;

-(IBAction) sliderChanged:(id)sender;
@end
