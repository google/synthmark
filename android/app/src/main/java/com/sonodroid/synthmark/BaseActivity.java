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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v4.view.ViewGroupCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

public class BaseActivity extends AppCompatActivity {
    private String TAG = "SynthMark";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onPause() {
        super.onPause();
        log("onPause() UNregister Broadcast receiver for notification");
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mReceiver);
    }

    @Override
    protected void onResume() {
        super.onResume();
        log("onResume() Register Broadcast receiver for notification");
        LocalBroadcastManager.getInstance(this).registerReceiver(mReceiver,
                new IntentFilter(AppObject.INTENT_NOTIFICATION));
    }

    public void setLogTag(String logTag) {
        TAG = logTag;
    }

    public void log(String message) {
        Log.v(TAG,message);
    }

    protected AppObject getApp() {
        return (AppObject) this.getApplication();
    }

    public void setViewEnable(View v, boolean enable) {
        v.setEnabled(enable);
        if (v instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) v;
            for (int i = 0; i < vg.getChildCount(); i++) {
                setViewEnable(vg.getChildAt(i), enable);
            }
        }
    }

    public void notificationTestStarted(int testId) {
        log("notificationTestStarted(" + testId + ")");
    }

    public void notificationTestUpdate(int testId, String message) {
        log("notificationTestUpdate(" + testId + ", \"" + message + "\")");
    }

    public void notificationTestCompleted(int testId) {
        log("notificationTestCompleted(" + testId + ")");
    }

    public void notificationTestShortUpdate(int testId, String message) {
        // log("notificationTestShortUpdate(" + testId + ", \"" + message + "\")");
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int type = intent.getIntExtra(AppObject.INTENT_NOTIFICATION_TYPE, -1);
            switch(type) {
                case AppObject.NOTIFICATION_TEST_STARTED: {
                    int testId = intent.getIntExtra(AppObject.NOTIFICATION_KEY_TEST_ID, -1);
                    notificationTestStarted(testId);
                }
                break;

                case AppObject.NOTIFICATION_TEST_UPDATE: {
                    int testId = intent.getIntExtra(AppObject.NOTIFICATION_KEY_TEST_ID, -1);
                    String message = intent.getStringExtra(AppObject.NOTIFICATION_KEY_MESSAGE);
                    notificationTestUpdate(testId, message);
                }
                break;

                case AppObject.NOTIFICATION_TEST_COMPLETED: {
                    int testId = intent.getIntExtra(AppObject.NOTIFICATION_KEY_TEST_ID, -1);
                    notificationTestCompleted(testId);
                }
                break;

                case AppObject.NOTIFICATION_TEST_SHORT_UPDATE: {
                    int testId = intent.getIntExtra(AppObject.NOTIFICATION_KEY_TEST_ID, -1);
                    String message = intent.getStringExtra(AppObject.NOTIFICATION_KEY_MESSAGE);
                    notificationTestShortUpdate(testId, message);
                }
                break;
            }
        }
    };
}
