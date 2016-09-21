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

#include <cstdint>
#include <math.h>
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/TimingAnalyzer.h"
#include "AudioSinkBase.h"
#include "tools/LogTool.h"
#include "SynthMarkResult.h"

#ifndef SYNTHMARK_SYNTHMARK_HARNESS_H
#define SYNTHMARK_SYNTHMARK_HARNESS_H


/**
 * Base class for running a test.
 */
class TestHarnessBase {
public:
    TestHarnessBase(AudioSinkBase *audioSink,
                    SynthMarkResult *result,
                    LogTool *logTool = NULL)
    : mSynth()
    , mAudioSink(NULL)
    , mTimer()
    , mLogTool(NULL)
    , mResult(result)
    , mSampleRate(SYNTHMARK_SAMPLE_RATE)
    , mSamplesPerFrame(2)
    , mFramesPerRender(0)
    , mNumVoices(8)
    , mFrameCounter(0)

    {
        mAudioSink = audioSink;

        if (!logTool) {
            mLogTool = new LogTool(this);
        } else {
            mLogTool = logTool;
        }
    }

    virtual ~TestHarnessBase() {
        if (mLogTool && mLogTool->getOwner() == this) {
            delete(mLogTool);
            mLogTool = NULL;
        }
    }

    // Customize the test by defining these virtual methods.
    virtual void onBeginMeasurement() = 0;

    virtual int32_t onBeforeNoteOn() = 0;

    virtual void onEndMeasurement() = 0;

    int32_t open(int32_t sampleRate,
            int32_t samplesPerFrame,
            int32_t framesPerRender,
            int32_t framesPerBurst
            ) {
        if (sampleRate < 8000) {
            mLogTool->log("ERROR in open, sampleRate too low = %d < 8000\n", sampleRate);
            return -1;
        }
        if (samplesPerFrame != 2) {
            mLogTool->log("ERROR in open, samplesPerFrame = %d != 2\n", samplesPerFrame);
            return -1;
        }
        if (framesPerRender < 1) {
            mLogTool->log("ERROR in open, framesPerRender too low = %d < 1\n", framesPerRender);
            return -1;
        }
        if (framesPerRender > SYNTHMARK_FRAMES_PER_RENDER) {
            mLogTool->log("ERROR in open, framesPerRender = %d > %d\n",
                framesPerRender, SYNTHMARK_FRAMES_PER_RENDER);
            return -1;
        }
        if (framesPerBurst < 8) {
            mLogTool->log("ERROR in open, framesPerBurst = %d < 8\n", framesPerRender);
            return -1;
        }
        mSampleRate = sampleRate;
        mSamplesPerFrame = samplesPerFrame;
        mFramesPerRender = framesPerRender;
        mFramesPerBurst = framesPerBurst;
        mRenderBuffer = new float[samplesPerFrame * framesPerRender];

        mSynth.setup(sampleRate, SYNTHMARK_MAX_VOICES);
        return mAudioSink->open(sampleRate, samplesPerFrame, framesPerBurst);
    }

    /**
     * Perform a SynthMark measurement. Results are stored in the SynthMarkResult object
     * passed in the constructor
     *
     * @param seconds - duration of test
     */
    void measure(double seconds) {

        assert(mResult != NULL);

        mLogTool->log("---- SynthMark version %d.%d ----\n", SYNTHMARK_MAJOR_VERSION,
                      SYNTHMARK_MINOR_VERSION);

        int32_t result; // Used to store the results of various operations during the test
        int32_t framesNeeded = (int)(mSampleRate * seconds);
        mFrameCounter = 0;
        mLogTool->setVar1(mFrameCounter);

        // Variables for turning notes on and off.
        bool areNotesOn = false;
        int32_t countdown = 0;
        const int32_t blocksOn = (int) (0.2 * mSampleRate / mFramesPerRender);
        const int32_t blocksOff = (int) (0.3 * mSampleRate / mFramesPerRender);

        onBeginMeasurement();

        result = mAudioSink->start();
        if (result < 0){
            mResult->setResultCode(SYNTHMARK_RESULT_AUDIO_SINK_START_FAILURE);
            return;
        }

        while (mFrameCounter < framesNeeded) {
            // Turn notes on and off so they never stop sounding.
            if (countdown <= 0) {
                if (areNotesOn) {
                    mSynth.allNotesOff();
                    countdown = blocksOff;
                    areNotesOn = false;
                } else {
                    result = onBeforeNoteOn();
                    if (result < 0) {
                        break;
                    }
                    result = mSynth.allNotesOn(mNumVoices);
                    if (result < 0) {
                        break;
                    }
                    countdown = blocksOn;
                    areNotesOn = true;
                }
            }
            countdown--;

            // Gather timing information.
            mTimer.markEntry();
            mSynth.renderStereo(mRenderBuffer, (int32_t) mFramesPerRender);
            // Ideally we should write the data earlier than when it is read,
            // by a full buffer's worth of time.
            int64_t fullFramePosition = mFrameCounter - mAudioSink->getBufferSizeInFrames();
            int64_t idealTime = mAudioSink->convertFrameToTime(fullFramePosition);
            mTimer.markExit(idealTime);

            // Output the audio using a blocking write.
            result = mAudioSink->write(mRenderBuffer, mFramesPerRender);
            if (result < 0){
                mResult->setResultCode(SYNTHMARK_RESULT_AUDIO_SINK_WRITE_FAILURE);
                return;
            }
            mFrameCounter += mFramesPerRender;
            mLogTool->setVar1(mFrameCounter);
        }
        mAudioSink->stop();

        onEndMeasurement();
    }

    int32_t close() {
        delete[] mRenderBuffer;
        return mAudioSink->close();
    }

    void setNumVoices(int32_t numVoices) {
        mNumVoices = numVoices;
    }

    int32_t getNumVoices() {
        return mNumVoices;
    }

protected:
    Synthesizer    mSynth;
    AudioSinkBase *mAudioSink;
    TimingAnalyzer mTimer;
    LogTool       *mLogTool;
    SynthMarkResult *mResult;
    std::string    mTestName;
    float         *mRenderBuffer;   // contains output of the synthesizer
    int32_t        mSampleRate;
    int32_t        mSamplesPerFrame;
    int32_t        mFramesPerRender; // number of frames rendered at one time
    int32_t        mFramesPerBurst;  // number of frames read by hardware at one time
    int32_t        mNumVoices;
    int32_t        mFrameCounter;
};


#endif // SYNTHMARK_SYNTHMARK_HARNESS_H
