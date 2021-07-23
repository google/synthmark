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

import android.Manifest;
import android.annotation.TargetApi;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;


public class MainActivity extends BaseActivity {
    private static final String KEY_TEST_NAME = "test";
    private static final String KEY_FILE_NAME = "file";
    private static final int MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE = 1001;

    private TextView mTextViewDeviceInfo, mTextViewOutput;
    private View.OnClickListener mButtonListener;
    private Switch mSwitchFakeTouches;
    private FakeKeyGenerator mKeyGenerator;
    private TextView mTextViewShortUpdate;

    private Spinner mSpinnerTest;
    private Button mButtonTest;
    private Button mButtonShare;
    private Button mButtonSettings;
    private ScrollView mScrollView;

    private boolean mTestRunningByIntent;
    private String mResultFileName;
    private Bundle mBundleFromIntent;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setLogTag("SynthMark");

        // Keep the screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Lock to portrait to avoid onCreate being called more than once
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        mScrollView = (ScrollView) findViewById(R.id.activity_main);
        mTextViewDeviceInfo = (TextView) findViewById(R.id.textViewDeviceInfo);

        mTextViewOutput = (TextView) findViewById(R.id.textViewOutput);
        mTextViewShortUpdate = (TextView) findViewById(R.id.textViewShortUpdate);

        mSwitchFakeTouches = (Switch) findViewById(R.id.switchFakeTouches);
        mKeyGenerator = FakeKeyGenerator.getInstance();

        // If running on N or above then enable the sustained performance option
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N){
            Switch s = (Switch) findViewById(R.id.switchSustainedPerformance);
            s.setEnabled(true);
            s.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                    setSustainedPerformanceMode(b);
                }
            });
        }

        mButtonTest = (Button) findViewById(R.id.buttonTest);
        mButtonShare = (Button) findViewById(R.id.buttonShare);
        mButtonShare.setEnabled(false);

        mSpinnerTest = (Spinner) findViewById(R.id.spinnerTest);
        List<String> spinnerList = new ArrayList<String>();

        int testCount = getApp().getTestCount();

        for (int i=0; i<testCount; i++) {
            String name = getApp().getTestName(i);
            spinnerList.add(name);
        }

        ArrayAdapter<String> spinnerAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, spinnerList);

        spinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mSpinnerTest.setAdapter(spinnerAdapter);

        if (testCount > 0) {
            getApp().setCurrentTestId(0);

            mSpinnerTest.setSelection(getApp().getCurrentTestId());
        }
        mSpinnerTest.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                log("Selected " + position);
                getApp().setCurrentTestId(position);
                logParams();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        mButtonSettings = (Button) findViewById(R.id.buttonSettings);

        saveIntentBundleForLaterProcessing(getIntent());
    }

    @TargetApi(Build.VERSION_CODES.N)
    private void setSustainedPerformanceMode(boolean enable){
        this.getWindow().setSustainedPerformanceMode(enable);
    }

    @Override
    public void onNewIntent(Intent intent) {
        log("onNewIntent()");
        saveIntentBundleForLaterProcessing(intent);
    }

    // Sample command
    // adb shell am start -n com.sonodroid.synthmark/.MainActivity --es test jitter \
    //       --ei core_affinity 1  --es file /sdcard/test20190611.txt
    //
    // Required parameters:
    //   -es test = { voice, util, jitter, latency, auto }
    //   -es file = full path for resulting file
    // Other optional parameters defined in NativeTest.h
    //   "sample_rate"
    //   "samples_per_frame"
    //   "frames_per_render"
    //   "frames_per_burst"
    //   "num_seconds"
    //   "note_on_delay"
    //   "target_cpu_load"
    //   "num_voices"
    //   "num_voices_high"
    //   "core_affinity"
    //
    // This will get processed during onResume.
    private void saveIntentBundleForLaterProcessing(Intent intent) {
        mBundleFromIntent = intent.getExtras();
    }

    private void processBundleFromIntent() {
        if (mBundleFromIntent == null) {
            return;
        }
        if (mTestRunningByIntent) {
            log("Previous test still running.");
            return;
        }

        // Dump key values for debugging.
        for (String key: mBundleFromIntent.keySet()) {
            log("Intent: " + key + " => " + mBundleFromIntent.get(key));
        }

        mResultFileName = null;
        if (mBundleFromIntent.containsKey(KEY_TEST_NAME)) {
            String testName = mBundleFromIntent.getString(KEY_TEST_NAME);
            int testId = extractTestId(testName);
            if (testId >= 0) {
                getApp().setCurrentTestId(testId);
                extractParametersFromBundle(mBundleFromIntent);
                mResultFileName = mBundleFromIntent.getString(KEY_FILE_NAME);
                mTestRunningByIntent = true;

                // Run the test using the parameters from the bundle.
                getApp().startTest(testId);
            }
        }
        mBundleFromIntent = null;
    }

    private int extractTestId(String testName) {
        testName = testName.toLowerCase();
        // Match "test" name to start of official Name.
        // So "test jitter" will match "JitterMark"
        int testId = -1;
        int testCount = getApp().getTestCount();
        for (int i = 0; i < testCount; i++) {
            String name = getApp().getTestName(i);
            log("Test: " + i + " =" + name);
            if (name.toLowerCase().startsWith(testName)) {
                testId = i;
                log("Test: matched " + testId);
                break;
            }
        }
        return testId;
    }

    private void extractParametersFromBundle(Bundle b) {
        List<Param> paramList = getApp().getParamsForTest();
        for (Param param : paramList) {
            String desc = param.getDescription();
            log("Param: " + desc);
            String name = param.getName();
            log("    name = " + name);
            if (b.containsKey(name)) {
                Object value = b.get(name);
                log("    value = " + value);
                param.setValueByObject(value);
            }
        }
    }

    void logParams() {
        final int currentTestId = getApp().getCurrentTestId();
        List<Param> paramList = getApp().getParamsForTest(currentTestId);

        for (Param param : paramList) {
            String desc = param.getDescription();
            log("Param: " + desc);
            log("    name = " + param.getName());
        }
    }

    @Override
    public void notificationTestStarted(int testId) {
        super.notificationTestStarted(testId);

        if (mSwitchFakeTouches.isChecked()){
            mKeyGenerator.start();
        }
        mTextViewOutput.setText("Starting test " + getApp().getTestName(testId) + "\n");

        // TODO: Add this to the result object
        mTextViewOutput.append(
                "Fake touches: " +
                (mSwitchFakeTouches.isChecked() ? "On" : "Off") +
                "\n");

        mTextViewShortUpdate.setText("run");

        //disable stuff
        mButtonSettings.setEnabled(false);
        mSpinnerTest.setEnabled(false);
        mButtonTest.setEnabled(false);
        mButtonShare.setEnabled(false);
    }

    @Override
    public void notificationTestUpdate(int testId, String message) {
        super.notificationTestUpdate(testId, message);
        mTextViewOutput.append(message);
        mScrollView.post(new Runnable() {
            public void run() {
                mScrollView.fullScroll(mScrollView.FOCUS_DOWN);
            }
        });
    }

    @Override
    public void notificationTestShortUpdate(int testId, String message) {
        super.notificationTestShortUpdate(testId, message);
        mTextViewShortUpdate.setText(message);
    }

    @Override
    public void notificationTestCompleted(int testId) {
        super.notificationTestCompleted(testId);
        mKeyGenerator.stop();
        mTextViewOutput.append("Finished test " + getApp().getTestName(testId) + "\n");

        //enable stuff
        mButtonSettings.setEnabled(true);
        mSpinnerTest.setEnabled(true);
        mButtonTest.setEnabled(true);
        mButtonShare.setEnabled(true);

        maybeWriteTestResult();
    }

    private void maybeWriteTestResult() {
        if (!mTestRunningByIntent || mResultFileName == null) return;
        writeTestResultIfPermitted();
    }

    void writeTestResultIfPermitted() {
        // Here, thisActivity is the current activity
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {

            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE);
        } else {
            // Permission has already been granted
            writeTestResult();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        switch (requestCode) {
            case MY_PERMISSIONS_REQUEST_EXTERNAL_STORAGE: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    writeTestResult();
                } else {
                    showToast("Writing external storage needed for test results.");
                }
                return;
            }
        }
    }

    private void writeTestInBackground() {
        new Thread() {
            public void run() {
                writeTestResult();
            }
        }.start();
    }

    // Run this in a background thread.
    private void writeTestResult() {
        File resultFile = new File(mResultFileName);
        String resultString = mTextViewOutput.getText().toString();
        Writer writer = null;
        try {
            writer = new OutputStreamWriter(new FileOutputStream(resultFile));
            writer.write(resultString);
        } catch (
                IOException e) {
            e.printStackTrace();
            showErrorToast(" writing result file. " + e.getMessage());
        } finally {
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        mTestRunningByIntent = false;
        mResultFileName = null;
    }

    public void onSettingsClicked(View view) {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }

    public void onRunTest(View view) {
        int currentTestId = getApp().getCurrentTestId();
        if (currentTestId > -1) {
            getApp().startTest(currentTestId);
        }
    }

    public void onShareResult(View view) {
        Intent sharingIntent = new Intent(android.content.Intent.ACTION_SEND);
        sharingIntent.setType("text/plain");

        String subjectText = "SynthMark result " + getTimestampString();
        sharingIntent.putExtra(android.content.Intent.EXTRA_SUBJECT, subjectText);

        String shareBody = mTextViewOutput.getText().toString();
        sharingIntent.putExtra(android.content.Intent.EXTRA_TEXT, shareBody);

        startActivity(Intent.createChooser(sharingIntent, "Share using:"));
    }

    private String getTimestampString() {
        DateFormat df = new SimpleDateFormat("yyyyMMdd-HHmmss");
        Date now = Calendar.getInstance().getTime();
        return df.format(now);
    }

    public void onWindowFocusChanged(boolean hasFocus){
        if (!hasFocus && getApp().getRunning()){
            mTextViewOutput.append("" +
                    "WARNING: Window lost focus during test - may affect test result.\n");
            mKeyGenerator.stop();
        }
    }

    @Override
    public void onResume(){
        super.onResume();
        mTextViewDeviceInfo.setText(AppObject.getDeviceInfo());
        processBundleFromIntent();
    }

    @Override
    public void onPause(){
        if (getApp().getRunning()){
            mTextViewOutput.append(
                    "WARNING: Activity paused during test - may affect test result.\n");
            mKeyGenerator.stop();
        }
        super.onPause();
    }

    protected void showErrorToast(String message) {
        showToast("Error: " + message);
    }

    protected void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this,
                        message,
                        Toast.LENGTH_SHORT).show();
            }
        });
    }
}
