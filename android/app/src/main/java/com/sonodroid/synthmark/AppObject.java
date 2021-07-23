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
import android.os.PowerManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

public class AppObject extends Application {
    public final static String TAG = "SynthMark";

    private final static int PROGRESS_REFRESH_RATE_MS = 300;
    private final static int PROGRESS_WARM_UP_WAIT = 300;

    private final static int NATIVETEST_STATUS_RUNNING = 2; //from NativeTest.h

    public final static int PARAM_INTEGER = 0; //From Params.h
    public final static int PARAM_FLOAT = 1;

    public final static int PARAM_HOLD_RANGE = 0; //From params.h
    public final static int PARAM_HOLD_LIST = 1;

    public static final String SCHEDTUNE_BOOST_FILE_NAME = "/dev/stune/rt/schedtune.boost";

    private int mCurrentTestId = -1;

    static {
        System.loadLibrary("native-lib");
    }
    public native long native_create();
    public native int testInit(long nativeTest, int testId);
    public native int testClose(long nativeTest);
    public native int testRun(long nativeTest);
    public native int testProgress(long nativeTest);
    public native int testStatus(long nativeTest);
    public native boolean testHasLogs(long nativeTest);
    public native String testReadLog(long nativeTest);

    private native int native_getTestCount(long nativeTest);
    private native String native_getTestName(long nativeTest, int testId);

    //params
    private native int native_getParamCount(long nativeTest, int testId);
    private native int native_getParamType(long nativeTest, int testId, int paramIndex);
    private native int native_getParamHoldType(long nativeTest, int testId, int paramIndex);
    private native int native_getParamListSize(long nativeTest, int testId, int paramIndex);
    private native int native_getParamListCurrentIndex(long nativeTest, int testId, int paramIndex);
    private native int native_getParamListDefaultIndex(long nativeTest, int testId, int paramIndex);
    private native int native_setParamListCurrentIndex(long nativeTest, int testId, int paramIndex,
                                                       int index);
    private native String native_getParamListNameFromIndex(long nativeTest, int testId,
                                                           int paramIndex, int index);
    private native int native_resetParamValue(long nativeTest, int testId, int paramIndex);
    private native String native_getParamName(long nativeTest, int testId, int paramIndex);
    private native String native_getparamDesc(long nativeTest, int testId, int paramIndex);

    //INT params
    private native int native_getParamIntMin(long nativeTest, int testId, int paramIndex);
    private native int native_getParamIntMax(long nativeTest, int testId, int paramIndex);
    private native int native_getParamIntValue(long nativeTest, int testId, int paramIndex);
    private native int native_getParamIntDefault(long nativeTest, int testId, int paramIndex);

    private native int native_setParamIntValue(long nativeTest, int testId, int paramIndex,
                                               int value);

    //float
    private native float native_getParamFloatMin(long nativeTest, int testId, int paramIndex);
    private native float native_getParamFloatMax(long nativeTest, int testId, int paramIndex);
    private native float native_getParamFloatValue(long nativeTest, int testId, int paramIndex);
    private native float native_getParamFloatDefault(long nativeTest, int testId, int paramIndex);

    private native int native_setParamFloatValue(long nativeTest, int testId, int paramIndex,
                                                 float value);


    private long mNativeTest = native_create(); //pointer to native test class

    public int getTestCount() {
        return native_getTestCount(mNativeTest);
    }

    public String getTestName(int testId) {
        return native_getTestName(mNativeTest, testId);
    }

    public int getParamCount(int testId) {
        return native_getParamCount(mNativeTest, testId);
    }

    public int getParamType(int testId, int paramIndex) {
        return native_getParamType(mNativeTest, testId, paramIndex);
    }

    public int getParamHoldType(int testId, int paramIndex) {
        return native_getParamHoldType(mNativeTest, testId, paramIndex);
    }

    public int getParamListSize(int testId, int paramIndex) {
        return native_getParamListSize(mNativeTest, testId, paramIndex);
    }

    public int getParamListCurrentIndex(int testId, int paramIndex) {
        return native_getParamListCurrentIndex(mNativeTest, testId, paramIndex);
    }

    public int getParamListDefaultIndex(int testId, int paramIndex) {
        return native_getParamListDefaultIndex(mNativeTest, testId, paramIndex);
    }

    public String getParamListNameFromIndex(int testId, int paramIndex, int index) {
        return native_getParamListNameFromIndex(mNativeTest, testId, paramIndex, index);
    }

    public int setParamListCurrentIndex(int testId, int paramIndex, int index) {
        return native_setParamListCurrentIndex(mNativeTest, testId, paramIndex, index);
    }

    public int resetParamValue(int testId, int paramIndex) {
        return native_resetParamValue(mNativeTest, testId, paramIndex);
    }

    public String getParamName(int testId, int paramIndex) {
        return native_getParamName(mNativeTest, testId, paramIndex);
    }

    public String getParamDesc(int testId, int paramIndex) {
        return native_getparamDesc(mNativeTest, testId, paramIndex);
    }

    public int getParamIntMin(int testId, int paramIndex) {
        return native_getParamIntMin(mNativeTest, testId, paramIndex);
    }

    public int getParamIntMax(int testId, int paramIndex) {
        return native_getParamIntMax(mNativeTest, testId, paramIndex);
    }

    public int getParamIntValue(int testId, int paramIndex) {
        return native_getParamIntValue(mNativeTest, testId, paramIndex);
    }

    public int getParamIntDefault(int testId, int paramIndex) {
        return native_getParamIntDefault(mNativeTest, testId, paramIndex);
    }

    public int setParamIntValue(int testId, int paramIndex, int value) {
        return native_setParamIntValue(mNativeTest, testId, paramIndex, value);
    }

    public float getParamFloatMin(int testId, int paramIndex) {
        return native_getParamFloatMin(mNativeTest, testId, paramIndex);
    }

    public float getParamFloatMax(int testId, int paramIndex) {
        return native_getParamFloatMax(mNativeTest, testId, paramIndex);
    }

    public float getParamFloatValue(int testId, int paramIndex) {
        return native_getParamFloatValue(mNativeTest, testId, paramIndex);
    }

    public float getParamFloatDefault(int testId, int paramIndex) {
        return native_getParamFloatDefault(mNativeTest, testId, paramIndex);
    }

    public int setParamFloatValue(int testId, int paramIndex, float value) {
        return native_setParamFloatValue(mNativeTest, testId, paramIndex, value);
    }

    public int getCurrentTestId() {
        return mCurrentTestId;
    }

    public void setCurrentTestId(int currentTestId) {
        mCurrentTestId = currentTestId;
    }

    List<Param> getParamsForTest(int testId) {
        List<Param> paramList = new ArrayList<Param>();
        int paramCount = getParamCount(testId);

        for (int i = 0; i < paramCount; i++) {
            paramList.add(new Param(getApplicationContext(), testId, i));
        }
        return paramList;
    }

    List<Param> getParamsForTest() {
        return getParamsForTest(getCurrentTestId());
    }

    @Override
    protected void finalize() throws Throwable {
        testClose(mNativeTest);
        super.finalize();
    }

    /**
     * Read contents of a file into a String.
     *
     * @param filename
     * @return contents of a text file
     */
    public static String readFile(String filename) {
        try {
            FileInputStream fis = new FileInputStream(filename);
            InputStreamReader isr = new InputStreamReader(fis, "UTF-8");
            BufferedReader bufferedReader = new BufferedReader(isr);
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                sb.append(line).append("\n");
            }
            return sb.toString();
        } catch (FileNotFoundException e) {
            return "";
        } catch (UnsupportedEncodingException e) {
            return "";
        } catch (IOException e) {
            return "";
        }
    }
    
    public static String getDeviceInfo(){

        StringBuffer info = new StringBuffer();

        info.append("SynthMark: " + BuildConfig.VERSION_NAME + "\n");
        info.append("Manufacturer: " + Build.MANUFACTURER + "\n");
        info.append("Model: " + Build.MODEL + "\n");
        info.append("ABI: " + Build.SUPPORTED_ABIS[0] + "\n");
        info.append("Android API: " + Build.VERSION.SDK_INT + " Build " + Build.ID + "\n");
        info.append("CPU cores: " + Runtime.getRuntime().availableProcessors() + "\n");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
            try {
                int[] exclusiveCoreIds = android.os.Process.getExclusiveCores();
                info.append("Exclusive cores IDs: ");
                for (int id : exclusiveCoreIds) {
                    info.append(id + ", ");
                }
            } catch(Exception e) {
                String message = "getExclusiveCores() CRASHED!";
                Log.e(TAG, message, e);
                info.append(message);
            }
            info.append("\n");
        }

        String boostInfo = readFile(SCHEDTUNE_BOOST_FILE_NAME).trim();
        if (boostInfo.length() == 0) {
            boostInfo = "----";
        }
        info.append("schedtune.boost: " + boostInfo + "\n");

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

            // Acquire a wakelock
            PowerManager powerManager = (PowerManager) getSystemService(POWER_SERVICE);
            PowerManager.WakeLock wakeLock = powerManager.newWakeLock(
                    PowerManager.PARTIAL_WAKE_LOCK, "SynthMark:WakeLock");
            wakeLock.acquire();

            try {
                Thread.sleep(PROGRESS_WARM_UP_WAIT);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            while (running) {
                if (mNativeTest != 0) {
                    // Limit the amount of progress display because it revs up the CPU.

                    int status = testStatus(mNativeTest);
                    if (status != NATIVETEST_STATUS_RUNNING) {
                        running = false;
                    }

                    try {
                        Thread.sleep(PROGRESS_REFRESH_RATE_MS);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                    updateOutputReport();
                } else {
                    break;
                }
            }

            // Release the wakelock
            wakeLock.release();

            return "done";
        }

        private void updateOutputReport() {
            if (testHasLogs(mNativeTest)) {
                String log = testReadLog(mNativeTest);
                postNotificationTestUpdate(mTestId, log);
            }
        }

        protected void onPostExecute(String result) {
            log("TestProgressTask.onPostExecute result: " + result);
            postNotificationTestShortUpdate(mTestId, result);
            postNotificationTestCompleted(mTestId);
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
            testInit(mNativeTest, mTestId);

            postNotificationTestUpdate(mTestId, "Test initialized\n");
            testRun(mNativeTest);
            setRunning(false);

            return "done"; // gets passed to onPostExecute()
        }

        protected void onPostExecute(String result) {
            log("TestTask.onPostExecute result: " + result);
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

    public void resetSettings(int testId) {
        int paramCount = getParamCount(testId);
        for (int i = 0; i<paramCount; i++) {
            resetParamValue(testId,i);
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
        // log("Post test started");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_STARTED);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestUpdate(int testId, String message) {
        int end = Math.min(message.length(), 40);
        // log("Post test update: #" + message.length() + ", " + message.substring(0, end));
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_UPDATE);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        intent.putExtra(NOTIFICATION_KEY_MESSAGE, message);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestCompleted(int testId) {
        // log("Post test completed");
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_COMPLETED);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    public void postNotificationTestShortUpdate(int testId, String message) {
        Intent intent = new Intent(AppObject.INTENT_NOTIFICATION);
        intent.putExtra(INTENT_NOTIFICATION_TYPE, NOTIFICATION_TEST_SHORT_UPDATE);
        intent.putExtra(NOTIFICATION_KEY_TEST_ID, testId);
        intent.putExtra(NOTIFICATION_KEY_MESSAGE, message);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }
}
