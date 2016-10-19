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

#import "ParamTableViewCell.h"
#import "AppDelegate.h"
#include "tools/NativeTest.h"

@implementation ParamTableViewCell
@synthesize mTestId;
@synthesize mParamId;
@synthesize m_appObject;
@synthesize labelValue;
@synthesize labelDescription;
@synthesize sliderValue;

- (void)awakeFromNib {
    [super awakeFromNib];
    // Initialization code
    AppDelegate *appDelegate = (AppDelegate *) [UIApplication sharedApplication].delegate;
    m_appObject = appDelegate.m_appObject;
    mTestId = -1;
    mParamId = -1;

}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

-(void) setTestId:(int) testId paramId:(int) paramId {
    mTestId = testId;
    mParamId = paramId;

    [self refreshScreen];
}

-(void) refreshScreen {

    ParamBase *pBase = [self getParamBase:mTestId paramId:mParamId];
    if (pBase != NULL) {
        NSString * desc = [[NSString alloc] initWithFormat:@"%s",pBase->getDescription().c_str()];
        [labelDescription setText:desc];

        int type = pBase->getType();
        int holdType = pBase->getHoldType();

        switch (holdType) {
            case ParamBase::PARAM_HOLD_LIST: {
                int maxIndex = pBase->getListSize() - 1;
                [sliderValue setMinimumValue:0];
                [sliderValue setMaximumValue:maxIndex];
            }
                break;
            case ParamBase::PARAM_HOLD_RANGE: {
                switch(type) {
                    case ParamBase::PARAM_INTEGER: {
                        ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                        if (pInt) {
                            int max = pInt->getMax();
                            int min = pInt->getMin();

                            [sliderValue setMaximumValue:max];
                            [sliderValue setMinimumValue:min];
                        }
                    }
                        break;
                    case ParamBase::PARAM_FLOAT: {
                        ParamFloat *pFloat = static_cast<ParamFloat*>(pBase);
                        if (pFloat) {
                            float max = pFloat->getMax();
                            float min = pFloat->getMin();

                            [sliderValue setMaximumValue:max];
                            [sliderValue setMinimumValue:min];
                        }
                    }
                        break;
                }
            }
                break;
        }
        [self updateScreenValue:true];
    }
}

-(void) updateScreenValue:(BOOL) updateSliders {
    ParamBase *pBase = [self getParamBase:mTestId paramId:mParamId];

    int type = pBase->getType();
    int holdType = pBase->getHoldType();

    switch (holdType) {
        case ParamBase::PARAM_HOLD_LIST: {
            int index = pBase->getListCurrentIndex();
            int defaultIndex = pBase->getListDefaultIndex();

            std::string sName;

            if (type == ParamBase::PARAM_INTEGER) {
                ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                if (pInt != NULL) {
                    sName = pInt->getParamNameFromList(index);
                }
            } else if (type == ParamBase::PARAM_FLOAT) {
                ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
                if (pFloat != NULL) {
                    sName = pFloat->getParamNameFromList(index);
                }
            }

            NSString * strValue = [[NSString alloc] initWithFormat:@"%s%s",
                                   sName.c_str(),
                                   index == defaultIndex ? "*":""];
            [labelValue setText:strValue];
            if (updateSliders) {
                [sliderValue setValue:index];
            }

        }
            break;
        case ParamBase::PARAM_HOLD_RANGE: {
            switch(type) {
                case ParamBase::PARAM_INTEGER: {
                    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                    if (pInt) {
                        int value = pInt->getValue();
                        int valueToDisplay = value;
                        int defaultValue = pInt->getDefaultValue();

                        NSString * strValue = [[NSString alloc] initWithFormat:@"%d%s",
                                               valueToDisplay,
                                               value == defaultValue ? "*":""];
                        [labelValue setText:strValue];
                        if (updateSliders) {
                            [sliderValue setValue:value];
                        }
                    }
                }
                    break;
                case ParamBase::PARAM_FLOAT: {
                    ParamFloat *pFloat = static_cast<ParamFloat*>(pBase);
                    if (pFloat) {
                        float value = pFloat->getValue();
                        float defaultValue = pFloat->getDefaultValue();
                        NSString * strValue = [[NSString alloc] initWithFormat:@"%0.3f%s",value,
                                               value == defaultValue ? "*":""];
                        [labelValue setText:strValue];
                        if (updateSliders) {
                            [sliderValue setValue:value];
                        }
                    }
                }
                    break;
            }
        }
            break;
    } //hold type
}

-(IBAction) sliderChanged:(id)sender {

    if (sender == sliderValue) {

        float value = sliderValue.value;

        ParamBase *pBase = [self getParamBase:mTestId paramId:mParamId];
        if (pBase != NULL) {
            int type = pBase->getType();
            int holdType = pBase->getHoldType();

            switch (holdType) {
                case ParamBase::PARAM_HOLD_LIST: {
                    pBase->setListCurrentIndex(value);
                }
                    break;
                case ParamBase::PARAM_HOLD_RANGE: {
                    switch(type) {
                        case ParamBase::PARAM_INTEGER: {
                            ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                            if (pInt) {
                                pInt->setValue(value);
                            }
                        }
                            break;
                        case ParamBase::PARAM_FLOAT: {
                            ParamFloat *pFloat = static_cast<ParamFloat*>(pBase);
                            if (pFloat) {
                                pFloat->setValue(value);
                            }
                            break;
                        }
                    }
                }
                    break;
            } //hold type
        }

        [self updateScreenValue:true];
    }
}


-(ParamBase*) getParamBase:(int) testId paramId:(int) paramId {

    NativeTest * pNativeTest = [m_appObject getNativeTest];
    if (pNativeTest == NULL) {
        return NULL;
    }

    ParamGroup *pGroup = pNativeTest->getParamGroup(testId);
    if (pGroup == NULL) {
        return NULL;
    }

    ParamBase *pBase = pGroup->getParamByIndex(paramId);
    return pBase;
}

@end
