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
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class SettingsActivity extends BaseActivity {
    private View.OnClickListener mButtonListener;
    private Button mButtonDone;
    private Button mButtonDefaults;
    private LinearLayout mLayoutSettings;

    List<Param> mParamList = new ArrayList<Param>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        mButtonListener = new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onButtonClicked(view);
            }
        };

        mButtonDone = (Button) findViewById(R.id.buttonDone);
        mButtonDone.setOnClickListener(mButtonListener);

        mButtonDefaults = (Button) findViewById(R.id.buttonDefaults);
        mButtonDefaults.setOnClickListener(mButtonListener);

        mLayoutSettings = (LinearLayout) findViewById(R.id.layoutSettings);
        refreshParams();
    }

    public void onButtonClicked(View view) {

        int id = view.getId();

        switch(id) {
            case R.id.buttonDone:
                finish();
                break;
            case R.id.buttonDefaults:
                getApp().resetSettings(getApp().getCurrentTestId());
                refreshParams();
                break;

        }
    }

    private class ParamSeekBar extends SeekBar {
        ParamSeekBar(Context context, final Param myParam, final TextView tvValue) {
            super(context);
            final int type = myParam.getType();//
            final int holdType = myParam.getHoldType();

            final Number nMin = myParam.getMin();
            final Number nMax = myParam.getMax();
            Number nValue = myParam.getValue();
            final int points = 10000; //points for slider if float

            if (holdType == AppObject.PARAM_HOLD_RANGE) {
                if (type == AppObject.PARAM_INTEGER) {
                    final int min = nMin.intValue();
                    final int max = nMax.intValue();
                    int value = nValue.intValue();
                    setMax(max - min);
                    setProgress(value - min);

                } else if (type == AppObject.PARAM_FLOAT) {
                    final float min = nMin.floatValue();
                    final float max = nMax.floatValue();
                    float value = nValue.floatValue();
                    setMax(points);
                    setProgress((int) (0.5 + value * points / (max - min)));
                }
            } else if (holdType == AppObject.PARAM_HOLD_LIST) {
                int min = 0;
                int max = myParam.getListSize() - 1;
                int currentIndex = myParam.getListCurrentIndex();
                setMax(max);
                setProgress(currentIndex);
            }

            tvValue.setText("  " + myParam.getValueAsString());

            setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                    if (holdType == AppObject.PARAM_HOLD_RANGE) {
                        if (type == AppObject.PARAM_INTEGER) {
                            int value = i + nMin.intValue();
                            myParam.setValue(new Integer(value));
                        } else if (type == AppObject.PARAM_FLOAT) {
                            float value = (nMax.floatValue() - nMin.floatValue()) * i / points;
                            myParam.setValue(new Float(value));
                        }

                    } else if (holdType == AppObject.PARAM_HOLD_LIST) {
                        myParam.setListCurrentIndex(i);
                    }
                    tvValue.setText("  " + myParam.getValueAsString());
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                }
            });
        }
    }

    private void refreshParams() {

        final int currentTestId = getApp().getCurrentTestId();
        mParamList.clear();
        mParamList = getApp().getParamsForTest(currentTestId);

        int paramCount = mParamList.size();
        mLayoutSettings.removeAllViews(); //remove all subviews

        for (int i = 0; i<paramCount; i++) {
            String desc = mParamList.get(i).getDescription();

            LinearLayout layoutOut = new LinearLayout(this);
            layoutOut.setOrientation(LinearLayout.VERTICAL);

            LinearLayout layoutTop = new LinearLayout(this);
            layoutTop.setOrientation(LinearLayout.HORIZONTAL);

            LinearLayout layoutBottom = new LinearLayout(this);
            layoutBottom.setOrientation(LinearLayout.HORIZONTAL);

            TextView tvTitle = new TextView(this);
            tvTitle.setText(desc);

            TextView tvValue = new TextView(this);
            tvValue.setTextAlignment(View.TEXT_ALIGNMENT_VIEW_END);

            tvValue.setLayoutParams(new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            layoutTop.addView(tvTitle);
            layoutTop.addView(tvValue);

            ParamSeekBar sbSlider = new ParamSeekBar(this, mParamList.get(i), tvValue);
            sbSlider.setLayoutParams(new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            layoutBottom.addView(sbSlider);

            layoutOut.addView(layoutTop);
            layoutOut.addView(layoutBottom);

            mLayoutSettings.addView(layoutOut);

        }
    }
}
