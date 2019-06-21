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

package com.sonodroid.synthmark;

import android.content.Context;

public class Param {
    private Context mContext;
    private int mTestId;
    private int mParamId;
    private int mType;
    private int mHoldType;

    private String mDescription;

    Param(Context context, int testId, int paramId) {
        mContext = context;
        mTestId = testId;
        mParamId = paramId;

        mDescription = getApp().getParamDesc(testId, paramId);
        mType = getApp().getParamType(testId, paramId);
        mHoldType = getApp().getParamHoldType(testId, paramId);

        //compute min/max
    }

    public int getTestId() {
        return mTestId;
    }

    public int getmParamId() {
        return mParamId;
    }

    public int getType() {
        return mType;
    }

    public int getHoldType() {
        return mHoldType;
    }

    public int getListSize() {
        return getApp().getParamListSize(mTestId, mParamId);
    }

    public int getListCurrentIndex() {
        return getApp().getParamListCurrentIndex(mTestId, mParamId);
    }

    public int getListDefaultIndex() {
        return getApp().getParamListDefaultIndex(mTestId, mParamId);
    }

    public String getName() {
        return getApp().getParamName(mTestId, mParamId);
    }

    public String getParamNameFromList(int index) {
        return getApp().getParamListNameFromIndex(mTestId, mParamId, index);
    }

    public int setListCurrentIndex(int index) {
        return getApp().setParamListCurrentIndex(mTestId, mParamId, index);
    }
    public String getDescription() {
        return mDescription;
    }

    public Number getMin() {
        Number result = null;
        if (mType == AppObject.PARAM_INTEGER) {
            result = new Integer(getApp().getParamIntMin(mTestId, mParamId));
        } else if (mType == AppObject.PARAM_FLOAT) {
            result = new Float(getApp().getParamFloatMin(mTestId, mParamId));
        }
        return result;
    }

    public Number getMax() {
        Number result = null;
        if (mType == AppObject.PARAM_INTEGER) {
            result = new Integer(getApp().getParamIntMax(mTestId, mParamId));
        } else if (mType == AppObject.PARAM_FLOAT) {
            result = new Float(getApp().getParamFloatMax(mTestId, mParamId));
        }
        return result;
    }

    public Number getValue() {
        Number result = null;
        if (mType == AppObject.PARAM_INTEGER) {
            result = new Integer(getApp().getParamIntValue(mTestId, mParamId));
        } else if (mType == AppObject.PARAM_FLOAT) {
            result = new Float(getApp().getParamFloatValue(mTestId, mParamId));
        }
        return result;
    }

    public String getValueAsString() {
        String result ="";
        if (mHoldType == AppObject.PARAM_HOLD_RANGE) {
            if (mType == AppObject.PARAM_INTEGER) {
                int value = getApp().getParamIntValue(mTestId, mParamId);
                int defaultValue = getApp().getParamIntDefault(mTestId, mParamId);
                result = String.format("%d%s", value, value == defaultValue ? "*" : "");
            } else if (mType == AppObject.PARAM_FLOAT) {
                float value = getApp().getParamFloatValue(mTestId, mParamId);
                float defaultValue = getApp().getParamFloatDefault(mTestId, mParamId);
                result = String.format("%.3f%s", value, value == defaultValue ? "*" : "");
            }
        } else if (mHoldType == AppObject.PARAM_HOLD_LIST) {
            int index = getListCurrentIndex();
            int defaultIndex = getListDefaultIndex();
            String name = getParamNameFromList(index);
            result = String.format("%s%s", name, index == defaultIndex ? "*" : "");
        }
        return result;
    }

    public void setValue(Number value) {
        if (mType == AppObject.PARAM_INTEGER) {
            getApp().setParamIntValue(mTestId, mParamId, value.intValue());
        } else if (mType == AppObject.PARAM_FLOAT) {
            getApp().setParamFloatValue(mTestId, mParamId, value.floatValue());
        }
    }

    public void setValueByObject(Object value) {
        if (mType == AppObject.PARAM_INTEGER) {
            getApp().setParamIntValue(mTestId, mParamId, ((Integer)value).intValue());
        } else if (mType == AppObject.PARAM_FLOAT) {
            getApp().setParamFloatValue(mTestId, mParamId, ((Float)value).floatValue());
        }
    }

    protected AppObject getApp() {

        return (AppObject) mContext.getApplicationContext();
    }
}
