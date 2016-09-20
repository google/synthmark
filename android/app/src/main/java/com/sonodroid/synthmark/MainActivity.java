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
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.ProgressBar;
import android.widget.Switch;
import android.widget.TextView;


public class MainActivity extends BaseActivity {
    TextView mTextViewDeviceInfo, mTextViewOutput;
    Button mButtonTest1, mButtonTest2, mButtonTest3;
    View.OnClickListener mButtonListener;
    Switch mSwitchFakeTouches;
    FakeKeyGenerator mKeyGenerator;
    ProgressBar mProgressBarRunning;
    TextView mTextViewShortUpdate;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setLogTag("MainActivity");

        // Lock to portrait to avoid onCreate being called more than once
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        mTextViewDeviceInfo = (TextView) findViewById(R.id.textViewDeviceInfo);
        mTextViewDeviceInfo.setText(AppObject.getDeviceInfo());

        mTextViewOutput = (TextView) findViewById(R.id.textViewOutput);
        mProgressBarRunning = (ProgressBar) findViewById(R.id.progressBarRunning);
        mTextViewShortUpdate = (TextView) findViewById(R.id.textViewShortUpdate);

        // Hook up the buttons
        mButtonTest1 = (Button) findViewById(R.id.buttonTest1);
        mButtonTest2 = (Button) findViewById(R.id.buttonTest2);
        mButtonTest3 = (Button) findViewById(R.id.buttonTest3);

        mButtonListener = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onButtonClicked(view);
            }
        };
        mButtonTest1.setOnClickListener(mButtonListener);
        mButtonTest2.setOnClickListener(mButtonListener);
        mButtonTest3.setOnClickListener(mButtonListener);

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
        mTextViewOutput.setText("Starting test " + testId + "\n");

        // TODO: Add this to the result object
        mTextViewOutput.append(
                "Fake touches: " +
                (mSwitchFakeTouches.isChecked() ? "On" : "Off") +
                "\n");
        mProgressBarRunning.setVisibility(View.VISIBLE);
    }

    @Override
    public void notificationTestUpdate(int testId, String message) {
        mTextViewOutput.append("[" + testId +"] " + message + "\n");
    }

    @Override
    public void notificationTestCompleted(int testId) {
        mKeyGenerator.stop();
        mTextViewOutput.append("Finished test " + testId + "\n");
        mProgressBarRunning.setVisibility(View.INVISIBLE);
    }

    @Override
    public void notificationTestShortUpdate(int testId, String message) {
        mTextViewShortUpdate.setText(message);
    }

    public void onButtonClicked(View view) {

        int id = view.getId();

        switch(id) {
            case R.id.buttonTest1:
                getApp().startTest(1);
                break;
            case R.id.buttonTest2:
                getApp().startTest(2);
                break;
            case R.id.buttonTest3:
                getApp().startTest(3);
                break;
        }
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
