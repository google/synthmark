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

import android.annotation.TargetApi;
import android.app.ActionBar;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;

import java.lang.reflect.Array;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;


public class MainActivity extends BaseActivity {
    private TextView mTextViewDeviceInfo, mTextViewOutput;
    private View.OnClickListener mButtonListener;
    private Switch mSwitchFakeTouches;
    private FakeKeyGenerator mKeyGenerator;
    private ProgressBar mProgressBarRunning;
    private TextView mTextViewShortUpdate;

    private Spinner mSpinnerTest;
    private Button mButtonTest;
    private Button mButtonShare;
    private Button mButtonSettings;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setLogTag("MainActivity");

        // Keep the screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Lock to portrait to avoid onCreate being called more than once
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        mTextViewDeviceInfo = (TextView) findViewById(R.id.textViewDeviceInfo);
        mTextViewDeviceInfo.setText(AppObject.getDeviceInfo());

        mTextViewOutput = (TextView) findViewById(R.id.textViewOutput);
        mProgressBarRunning = (ProgressBar) findViewById(R.id.progressBarRunning);
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
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        mButtonSettings = (Button) findViewById(R.id.buttonSettings);
    }

    @TargetApi(Build.VERSION_CODES.N)
    private void setSustainedPerformanceMode(boolean enable){
        this.getWindow().setSustainedPerformanceMode(enable);
    }

    @Override
    public void notificationTestStarted(int testId) {

        if (mSwitchFakeTouches.isChecked()){
            mKeyGenerator.start();
        }
        mTextViewOutput.setText("Starting test " + getApp().getTestName(testId) + "\n");

        // TODO: Add this to the result object
        mTextViewOutput.append(
                "Fake touches: " +
                (mSwitchFakeTouches.isChecked() ? "On" : "Off") +
                "\n");
        mProgressBarRunning.setVisibility(View.VISIBLE);

        //disable stuff
        mButtonSettings.setEnabled(false);
        mSpinnerTest.setEnabled(false);
        mButtonTest.setEnabled(false);
        mButtonShare.setEnabled(false);
    }

    @Override
    public void notificationTestUpdate(int testId, String message) {
        mTextViewOutput.append(message + "\n");
    }

    @Override
    public void notificationTestCompleted(int testId) {
        mKeyGenerator.stop();
        mTextViewOutput.append("Finished test " + getApp().getTestName(testId) + "\n");
        mProgressBarRunning.setVisibility(View.INVISIBLE);

        //enable stuff
        mButtonSettings.setEnabled(true);
        mSpinnerTest.setEnabled(true);
        mButtonTest.setEnabled(true);
        mButtonShare.setEnabled(true);
    }

    @Override
    public void notificationTestShortUpdate(int testId, String message) {
        mTextViewShortUpdate.setText(message);
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

    public void onPause(){

        if (getApp().getRunning()){
            mTextViewOutput.append(
                    "WARNING: Activity paused during test - may affect test result.\n");
            mKeyGenerator.stop();
        }
        super.onPause();
    }
}
