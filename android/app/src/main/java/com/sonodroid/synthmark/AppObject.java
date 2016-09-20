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

import android.app.Application;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Build;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

public class AppObject extends Application {
    public final static String TAG = "SynthMark";

    private final static int PROGRESS_REFRESH_RATE_MS = 300;
    private final static int PROGRESS_WARM_UP_WAIT = 300;

    private final static int NATIVETEST_STATUS_RUNNING = 2; //from NativeTest.h

    static {
        System.loadLibrary("native-lib");
    }
    public native long testInit(int testId);
    public native int testClose(long nativeTest);
    public native int testRun(long nativeTest);
    public native int testProgress(long nativeTest);
    public native int testStatus(long nativeTest);
    public native String testResult(long nativeTest);

    private long mNativeTest = 0; //pointer to native test class

    public static String getDeviceInfo(){

        StringBuffer info = new StringBuffer();

        info.append("Manufacturer " + Build.MANUFACTURER + "\n");
        info.append("Model " + Build.MODEL + "\n");
        info.append("ABI " + Build.SUPPORTED_ABIS[0] + "\n");
        info.append("Android API " + Build.VERSION.SDK_INT + " Build " + Build.ID + "\n");
        info.append("CPU cores " + Runtime.getRuntime().availableProcessors() + "\n");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
            int[] exclusiveCoreIds = android.os.Process.getExclusiveCores();
            info.append("Exclusive cores " + exclusiveCoreIds.length + "\n");
        }

        return info.toString();
    }

    public void log(String message) {
        Log.v(TAG,message);
    }
    private static boolean mRunning = false;
    private static final Object mRunningLock = new Object();

    public void setRunning(boolean running) {
        synchronized (mRunningLock) {
            mRunning = running;
        }
    }

    public boolean getRunning() {
        synchronized (mRunningLock) {
            return mRunning;
        }
    }

    public void startTest(int testId) {
        postTestUpdate(testId);
        new TestTask(testId) {}.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR,"");

        new TestProgressTask(testId) {}.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR,"");
    }

    private abstract class TestProgressTask extends AsyncTask<String, String, String> {
        boolean running = false;
        int mTestId;
        protected TestProgressTask(int testId) {
            mTestId = testId;
            running = true;
        }

        protected String doInBackground(String... params) {
            log("Started running progress...");
            try {
                Thread.sleep(PROGRESS_WARM_UP_WAIT);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            while (running) {
                log("r "+ mNativeTest);
                if (mNativeTest != 0) {
                    int progress = testProgress(mNativeTest);
                    int status = testStatus(mNativeTest);
                    String message = String.format("%d",progress);

                    postNotificationTestShortUpdate(mTestId, message);


                    if (status != NATIVETEST_STATUS_RUNNING) {
                        running = false;
                    }
                    try {
                        Thread.sleep(PROGRESS_REFRESH_RATE_MS);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                } else {
                    break;
                }
            }
            return "done";
        }

        protected void onPostExecute(String result) {
            log("done with progress");
            postNotificationTestShortUpdate(mTestId, "Done");
        }
    }




    private abstract class TestTask extends AsyncTask<String, String, String> {
        int mTestId;
        protected TestTask(int testId) {
            mTestId = testId;
        }

        protected String doInBackground(String... params) {
            setRunning(true);
            postNotificationTestStarted(mTestId);
            log("Started running test... ["+mTestId +"]");
            mNativeTest = testInit(mTestId);

            postNotificationTestUpdate(mTestId, "Test inited");
            testRun(mNativeTest);
            String result = testResult(mNativeTest);
            postNotificationTestUpdate(mTestId, result);
            testClose(mNativeTest);

            mNativeTest = 0;

            return result;
        }

        protected void onPostExecute(String result) {
            //mTextViewOutput.setText(result);
            log("onPostExecute result: " + result);
            postNotificationTestCompleted(mTestId);
            setRunning(false);
        }
    }

    private void postTestUpdate(int testId) {
        if (getRunning()) {
            //complain it is running.
            String msg = "Can't start test when another test is running";
            log(msg);
            postNotificationTestUpdate(testId, msg);
            return;
        }
    }

    public static final String INTENT_NOTIFICATION = "AppObjectEvent";
    public static final String INTENT_NOTIFICATION_TYPE = "AppObjectEvent_Type";

    public static final int NOTIFICATION_TEST_STARTED = 1;
    public static final int NOTIFICATION_TEST_UPDATE = 2;
    public static final int NOTIFICATION_TEST_COMPLETED = 3;
    public static final int NOTIFICATION_TEST_SHORT_UPDATE = 4;


    public static final String NOTIFICATION_KEY_TEST_ID = "TestId";
    public static final String NOTIFICATION_KEY_MESSAGE = "Message";

    public void postNotificationTestStarted(int testId) {
        log("Post test started");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_STARTED);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestUpdate(int testId, String message) {
        log("test update");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_UPDATE);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        intent.putExtra(NOTIFICATION_KEY_MESSAGE, message);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestCompleted(int testId) {
        log("Test ended");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_COMPLETED);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestShortUpdate(int testId, String message) {
        log("test update");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_SHORT_UPDATE);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        intent.putExtra(NOTIFICATION_KEY_MESSAGE, message);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }
}
